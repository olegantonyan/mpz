#include "playback/native/engine.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
#include <libavutil/mathematics.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include "miniaudio.h"

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstring>
#include <map>
#include <thread>
#include <vector>

namespace Playback {
  namespace Native {

    namespace {
      constexpr ma_uint32 OUT_CHANNELS = 2;
      constexpr ma_format OUT_FORMAT = ma_format_f32;
      constexpr double RB_SECONDS = 2.0;     // ring-buffer capacity
      constexpr int PRODUCE_SLEEP_MS = 5;    // backpressure / drain poll interval

      int64_t framesForMs(int64_t ms, ma_uint32 rate) {
        return ms * static_cast<int64_t>(rate) / 1000;
      }
    } // namespace

    struct Engine::Impl {
      // ---- miniaudio output ----
      ma_context context{};
      bool context_inited = false;
      ma_device device{};
      bool device_inited = false;
      ma_pcm_rb rb{};
      bool rb_inited = false;
      ma_uint32 out_rate = 48000;

      // ---- FFmpeg decode (touched only by the decode thread once running) ----
      AVFormatContext *fmt = nullptr;
      AVCodecContext *codec = nullptr;
      SwrContext *swr = nullptr;
      AVPacket *pkt = nullptr;
      AVFrame *frame = nullptr;
      int audio_index = -1;
      AVRational stream_tb{0, 1};
      int in_rate = 0;

      int64_t start_ms = 0;       // initial offset; only used on open / play-from-stop
      int64_t duration_ms = 0;
      bool seekable = false;

      // ---- threading / state ----
      std::thread decode_thread;
      std::atomic<bool> quit{false};
      std::atomic<bool> eof{false};
      std::atomic<bool> abort_io{false};   // unblocks an in-flight network read on stop
      std::atomic<State> state{State::Stopped};
      std::atomic<uint64_t> frames_played{0};   // output frames == absolute source position
      std::atomic<float> volume{1.0f};

      std::function<void()> on_eos;
      std::function<void(const std::string &)> on_error;
      std::function<void(const std::map<std::string, std::string> &)> on_meta;

      std::vector<uint8_t> out_buf;   // resample scratch (decode-thread only)

      ~Impl() { teardownAll(); }

      void reportError(const std::string &m) { if (on_error) on_error(m); }

      bool initContext();
      bool initDevice(const ma_device_id *id);
      void uninitDevice();
      bool initRingBuffer();
      std::string serializeId(const ma_device_id &id) const;
      bool switchDevice(const ma_device_id *id);   // recreate device, resume position

      static void dataCallback(ma_device *dev, void *out, const void *in, ma_uint32 frames);
      void onData(float *out, ma_uint32 frames);

      bool openInput(const std::string &url, int64_t start);
      void closeInput();
      bool setupResampler();        // (re)create swr for the current codec + out_rate
      void doSeekFile(int64_t abs_ms);

      void startDecodeThread();
      void stopDecodeThread();
      void decodeLoop();
      bool enqueueFrame(AVFrame *f);
      bool writeAll(const float *data, ma_uint32 frames);
      static int interruptCb(void *opaque);
      void extractIcyMeta(bool include_headers);

      void setFramesForMs(int64_t ms) {
        frames_played.store(static_cast<uint64_t>(framesForMs(ms, out_rate)), std::memory_order_relaxed);
      }
      int64_t filePosMs() const {
        return static_cast<int64_t>(frames_played.load(std::memory_order_relaxed)) * 1000 /
               (out_rate ? out_rate : 48000);
      }

      void teardownAll();
    };

    // ---------------------------------------------------------------- output --

    bool Engine::Impl::initContext() {
      if (context_inited) return true;
      if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        reportError("audio context init failed");
        return false;
      }
      context_inited = true;
      return true;
    }

    bool Engine::Impl::initRingBuffer() {
      if (rb_inited) { ma_pcm_rb_uninit(&rb); rb_inited = false; }
      ma_uint32 cap = static_cast<ma_uint32>(RB_SECONDS * out_rate);
      if (ma_pcm_rb_init(OUT_FORMAT, OUT_CHANNELS, cap, nullptr, nullptr, &rb) != MA_SUCCESS) {
        reportError("ring buffer init failed");
        return false;
      }
      rb_inited = true;
      return true;
    }

    bool Engine::Impl::initDevice(const ma_device_id *id) {
      uninitDevice();
      ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
      cfg.playback.pDeviceID = id;            // nullptr == system default
      cfg.playback.format = OUT_FORMAT;
      cfg.playback.channels = OUT_CHANNELS;
      cfg.sampleRate = 0;                      // use the device's native rate
      cfg.dataCallback = &Engine::Impl::dataCallback;
      cfg.pUserData = this;
      if (ma_device_init(context_inited ? &context : nullptr, &cfg, &device) != MA_SUCCESS) {
        reportError("audio device init failed");
        return false;
      }
      device_inited = true;
      out_rate = device.sampleRate ? device.sampleRate : 48000;
      ma_device_set_master_volume(&device, volume.load());
      return initRingBuffer();
    }

    void Engine::Impl::uninitDevice() {
      if (device_inited) { ma_device_uninit(&device); device_inited = false; }
    }

    std::string Engine::Impl::serializeId(const ma_device_id &id) const {
      // Hex of the raw ma_device_id. miniaudio zero-initialises device_info
      // during enumeration, so the inactive union bytes are stable, making this
      // a deterministic per-device key across runs on the same backend.
      const unsigned char *b = reinterpret_cast<const unsigned char *>(&id);
      static const char hexd[] = "0123456789abcdef";
      std::string s;
      s.reserve(sizeof(id) * 2);
      for (size_t i = 0; i < sizeof(id); ++i) {
        s.push_back(hexd[b[i] >> 4]);
        s.push_back(hexd[b[i] & 0x0f]);
      }
      return s;
    }

    bool Engine::Impl::switchDevice(const ma_device_id *id) {
      State prev = state.load();
      bool active = (prev == State::Playing || prev == State::Paused);
      if (active) {
        stopDecodeThread();
        if (device_inited && ma_device_is_started(&device)) ma_device_stop(&device);
      }
      bool ok = initDevice(id);
      if (ok && active) {
        // resume from the current position (frames_played preserved)
        int64_t resume_ms = filePosMs();
        doSeekFile(resume_ms);
        setFramesForMs(resume_ms);
        eof.store(false);
        startDecodeThread();
        if (prev == State::Playing) { ma_device_start(&device); state.store(State::Playing); }
        else { state.store(State::Paused); }
      }
      return ok;
    }

    void Engine::Impl::dataCallback(ma_device *dev, void *out, const void *, ma_uint32 frames) {
      static_cast<Impl *>(dev->pUserData)->onData(static_cast<float *>(out), frames);
    }

    // Realtime context: ring buffer + atomics only. No locks, no allocation, no Qt.
    void Engine::Impl::onData(float *out, ma_uint32 frames) {
      ma_uint32 need = frames;
      while (need > 0) {
        ma_uint32 n = need;
        void *src = nullptr;
        if (ma_pcm_rb_acquire_read(&rb, &n, &src) != MA_SUCCESS || n == 0) break;
        std::memcpy(out, src, static_cast<size_t>(n) * OUT_CHANNELS * sizeof(float));
        ma_pcm_rb_commit_read(&rb, n);
        out += static_cast<size_t>(n) * OUT_CHANNELS;
        need -= n;
        frames_played.fetch_add(n, std::memory_order_relaxed);
      }
      if (need > 0) {
        std::memset(out, 0, static_cast<size_t>(need) * OUT_CHANNELS * sizeof(float));
      }
    }

    // --------------------------------------------------------------- decode ---

    bool Engine::Impl::openInput(const std::string &url, int64_t start) {
      stopDecodeThread();
      if (device_inited && ma_device_is_started(&device)) ma_device_stop(&device);
      closeInput();

      start_ms = start;

      fmt = avformat_alloc_context();
      if (fmt == nullptr) { reportError("alloc format context failed"); return false; }
      fmt->interrupt_callback.callback = &Engine::Impl::interruptCb;
      fmt->interrupt_callback.opaque = this;

      AVDictionary *opts = nullptr;
      const bool is_http = url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0;
      if (is_http) {
        av_dict_set(&opts, "icy", "1", 0);                       // request ICY (now-playing) metadata
        av_dict_set(&opts, "user_agent", "mpz", 0);
        av_dict_set(&opts, "reconnect", "1", 0);
        av_dict_set(&opts, "reconnect_streamed", "1", 0);
        av_dict_set(&opts, "reconnect_delay_max", "5", 0);
        av_dict_set(&opts, "rw_timeout", "15000000", 0);         // 15s, avoid hanging the GUI on open
      }
      int rc = avformat_open_input(&fmt, url.c_str(), nullptr, &opts);
      av_dict_free(&opts);
      // On failure avformat_open_input frees and nulls fmt.
      if (rc < 0) {
        reportError("cannot open: " + url);
        return false;
      }

      if (avformat_find_stream_info(fmt, nullptr) < 0) {
        reportError("could not read stream info");
        closeInput();
        return false;
      }

      const AVCodec *dec = nullptr;
      audio_index = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
      if (audio_index < 0 || dec == nullptr) {
        reportError("no audio stream found");
        closeInput();
        return false;
      }

      AVStream *st = fmt->streams[audio_index];
      stream_tb = st->time_base;

      codec = avcodec_alloc_context3(dec);
      if (codec == nullptr) { reportError("codec alloc failed"); closeInput(); return false; }
      if (avcodec_parameters_to_context(codec, st->codecpar) < 0) {
        reportError("codec parameters failed");
        closeInput();
        return false;
      }
      if (avcodec_open2(codec, dec, nullptr) < 0) {
        reportError("codec open failed");
        closeInput();
        return false;
      }
      in_rate = codec->sample_rate;
      if (in_rate <= 0) { reportError("invalid sample rate"); closeInput(); return false; }

      if (!setupResampler()) {
        closeInput();
        return false;
      }

      pkt = av_packet_alloc();
      frame = av_frame_alloc();
      if (pkt == nullptr || frame == nullptr) { reportError("alloc failed"); closeInput(); return false; }

      if (fmt->duration > 0 && fmt->duration != AV_NOPTS_VALUE) {
        duration_ms = fmt->duration / (AV_TIME_BASE / 1000);
      } else {
        duration_ms = 0;
      }
      seekable = (fmt->pb != nullptr) && ((fmt->pb->seekable & AVIO_SEEKABLE_NORMAL) != 0);

      if (is_http) extractIcyMeta(true);   // initial ICY headers + now-playing title

      setFramesForMs(start_ms);
      eof.store(false);
      if (start_ms > 0) doSeekFile(start_ms);

      state.store(State::Stopped);
      return true;
    }

    void Engine::Impl::closeInput() {
      if (swr) { swr_free(&swr); swr = nullptr; }
      if (codec) { avcodec_free_context(&codec); codec = nullptr; }
      if (fmt) { avformat_close_input(&fmt); fmt = nullptr; }
      if (pkt) { av_packet_free(&pkt); pkt = nullptr; }
      if (frame) { av_frame_free(&frame); frame = nullptr; }
      audio_index = -1;
      duration_ms = 0;
      seekable = false;
    }

    bool Engine::Impl::setupResampler() {
      if (swr) { swr_free(&swr); swr = nullptr; }
      if (codec == nullptr) return false;
      AVChannelLayout in_layout;
      std::memset(&in_layout, 0, sizeof(in_layout));
      if (codec->ch_layout.nb_channels > 0) {
        av_channel_layout_copy(&in_layout, &codec->ch_layout);
      } else {
        av_channel_layout_default(&in_layout, 2);
      }
      AVChannelLayout out_layout;
      std::memset(&out_layout, 0, sizeof(out_layout));
      av_channel_layout_default(&out_layout, OUT_CHANNELS);
      int sret = swr_alloc_set_opts2(&swr, &out_layout, AV_SAMPLE_FMT_FLT,
                                     static_cast<int>(out_rate), &in_layout,
                                     codec->sample_fmt, in_rate, 0, nullptr);
      av_channel_layout_uninit(&in_layout);
      av_channel_layout_uninit(&out_layout);
      if (sret < 0 || swr == nullptr || swr_init(swr) < 0) {
        reportError("resampler init failed");
        return false;
      }
      return true;
    }

    void Engine::Impl::doSeekFile(int64_t abs_ms) {
      if (fmt == nullptr || audio_index < 0) return;
      int64_t ts = av_rescale_q(abs_ms, AVRational{1, 1000}, stream_tb);
      if (avformat_seek_file(fmt, audio_index, INT64_MIN, ts, ts, 0) < 0) {
        av_seek_frame(fmt, audio_index, ts, AVSEEK_FLAG_BACKWARD);
      }
      if (codec) avcodec_flush_buffers(codec);
      // Drop the resampler's pre-seek buffered samples; otherwise they are
      // emitted at the new position as a short burst of distorted audio. This
      // also rebuilds swr for the current out_rate, which matters when the
      // output device (and thus its sample rate) changed.
      setupResampler();
    }

    void Engine::Impl::startDecodeThread() {
      quit.store(false);
      decode_thread = std::thread([this] { decodeLoop(); });
    }

    void Engine::Impl::stopDecodeThread() {
      abort_io.store(true);    // unblock any in-flight network read so join() returns
      quit.store(true);
      if (decode_thread.joinable()) decode_thread.join();
      abort_io.store(false);   // GUI-thread avio (open/seek) must not be interrupted
    }

    int Engine::Impl::interruptCb(void *opaque) {
      return static_cast<Impl *>(opaque)->abort_io.load() ? 1 : 0;
    }

    void Engine::Impl::extractIcyMeta(bool include_headers) {
      if (!on_meta || fmt == nullptr) return;
      std::map<std::string, std::string> meta;
      auto trim = [](std::string &s) {
        while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
        size_t i = 0;
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
        s.erase(0, i);
      };
      if (include_headers) {
        uint8_t *headers = nullptr;
        if (av_opt_get(fmt, "icy_metadata_headers", AV_OPT_SEARCH_CHILDREN, &headers) >= 0 && headers) {
          std::string block(reinterpret_cast<const char *>(headers));
          size_t pos = 0;
          while (pos < block.size()) {
            size_t eol = block.find('\n', pos);
            std::string line = block.substr(pos, eol == std::string::npos ? std::string::npos : eol - pos);
            pos = (eol == std::string::npos) ? block.size() : eol + 1;
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
              std::string key = line.substr(0, colon);
              std::string val = line.substr(colon + 1);
              trim(key);
              trim(val);
              for (auto &c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
              if (!key.empty()) meta[key] = val;
            }
          }
          av_free(headers);
        }
      }
      uint8_t *packets = nullptr;
      if (av_opt_get(fmt, "icy_metadata_packets", AV_OPT_SEARCH_CHILDREN, &packets) >= 0 && packets) {
        if (packets[0] != '\0') meta["stream"] = reinterpret_cast<const char *>(packets);
        av_free(packets);
      }
      if (!meta.empty()) on_meta(meta);
    }

    bool Engine::Impl::writeAll(const float *data, ma_uint32 frames) {
      ma_uint32 off = 0;
      while (frames > 0) {
        if (quit.load()) return false;
        ma_uint32 n = frames;
        void *dst = nullptr;
        if (ma_pcm_rb_acquire_write(&rb, &n, &dst) != MA_SUCCESS) return false;
        if (n == 0) {   // buffer full -> wait for the device to drain
          std::this_thread::sleep_for(std::chrono::milliseconds(PRODUCE_SLEEP_MS));
          continue;
        }
        std::memcpy(dst, data + static_cast<size_t>(off) * OUT_CHANNELS,
                    static_cast<size_t>(n) * OUT_CHANNELS * sizeof(float));
        ma_pcm_rb_commit_write(&rb, n);
        off += n;
        frames -= n;
      }
      return true;
    }

    bool Engine::Impl::enqueueFrame(AVFrame *f) {
      int max_out = static_cast<int>(av_rescale_rnd(
          swr_get_delay(swr, in_rate) + f->nb_samples, out_rate, in_rate, AV_ROUND_UP));
      if (max_out <= 0) return true;
      size_t need = static_cast<size_t>(max_out) * OUT_CHANNELS * sizeof(float);
      if (out_buf.size() < need) out_buf.resize(need);

      uint8_t *outp[1] = {out_buf.data()};
      int got = swr_convert(swr, outp, max_out,
                            (const uint8_t **)f->extended_data, f->nb_samples);
      if (got < 0) { reportError("resample failed"); return false; }
      if (got == 0) return true;

      return writeAll(reinterpret_cast<const float *>(out_buf.data()), static_cast<ma_uint32>(got));
    }

    void Engine::Impl::decodeLoop() {
      bool finished = false;
      while (!quit.load()) {
        int ret = av_read_frame(fmt, pkt);
        if (ret < 0) {
          if (ret == AVERROR_EOF) {
            avcodec_send_packet(codec, nullptr);   // enter draining mode
          } else if (ret == AVERROR_EXIT || quit.load()) {
            break;   // interrupted by stop / seek / track change
          } else {
            reportError("read error");
            break;
          }
        } else {
          // Mid-stream ICY metadata change (radio "now playing").
          if (fmt->event_flags & AVFMT_EVENT_FLAG_METADATA_UPDATED) {
            fmt->event_flags &= ~AVFMT_EVENT_FLAG_METADATA_UPDATED;
            extractIcyMeta(false);
          }
          if (pkt->stream_index != audio_index) { av_packet_unref(pkt); continue; }
          avcodec_send_packet(codec, pkt);
          av_packet_unref(pkt);
        }

        bool stop_outer = false;
        while (!quit.load()) {
          int r = avcodec_receive_frame(codec, frame);
          if (r == AVERROR(EAGAIN)) break;
          if (r == AVERROR_EOF) { finished = true; stop_outer = true; break; }
          if (r < 0) { reportError("decode error"); stop_outer = true; break; }
          bool cont = enqueueFrame(frame);
          av_frame_unref(frame);
          if (!cont) { stop_outer = true; break; }   // writeAll saw quit
        }
        if (stop_outer) break;
      }

      eof.store(true);
      if (finished && !quit.load()) {
        // Let the device play out everything we produced before signalling EOS.
        while (!quit.load() && rb_inited && ma_pcm_rb_available_read(&rb) > 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(PRODUCE_SLEEP_MS));
        }
        if (!quit.load()) {
          state.store(State::Stopped);
          if (on_eos) on_eos();
        }
      }
    }

    void Engine::Impl::teardownAll() {
      stopDecodeThread();
      uninitDevice();
      if (rb_inited) { ma_pcm_rb_uninit(&rb); rb_inited = false; }
      closeInput();
      if (context_inited) { ma_context_uninit(&context); context_inited = false; }
    }

    // ----------------------------------------------------------- public API ---

    Engine::Engine() : d(std::make_unique<Impl>()) {
      // Quiet FFmpeg's stderr chatter (e.g. "Could not find codec parameters for
      // stream N (Video: mjpeg)" from embedded cover art in an audio-only build).
      // Genuine failures still reach us via return codes + the error callback.
      av_log_set_level(AV_LOG_ERROR);
      d->initContext();
      d->initDevice(nullptr);
    }

    Engine::~Engine() = default;

    void Engine::setEndOfStreamCallback(std::function<void()> cb) { d->on_eos = std::move(cb); }
    void Engine::setErrorCallback(std::function<void(const std::string &)> cb) { d->on_error = std::move(cb); }
    void Engine::setMetadataCallback(std::function<void(const std::map<std::string, std::string> &)> cb) {
      d->on_meta = std::move(cb);
    }

    bool Engine::open(const std::string &url, int64_t start_ms) {
      return d->openInput(url, start_ms);
    }

    void Engine::play() {
      Impl *p = d.get();
      State s = p->state.load();
      if (s == State::Playing) return;
      if (s == State::Paused) {
        if (p->device_inited) ma_device_start(&p->device);
        p->state.store(State::Playing);
        return;
      }
      // from Stopped: (re)start from the configured offset
      if (p->fmt == nullptr || !p->device_inited || !p->rb_inited) return;
      p->doSeekFile(p->start_ms);
      p->setFramesForMs(p->start_ms);
      ma_pcm_rb_reset(&p->rb);
      p->eof.store(false);
      p->startDecodeThread();
      if (ma_device_start(&p->device) != MA_SUCCESS) {
        p->reportError("audio device start failed");
        p->stopDecodeThread();
        return;
      }
      p->state.store(State::Playing);
    }

    void Engine::pause() {
      Impl *p = d.get();
      if (p->state.load() != State::Playing) return;
      if (p->device_inited) ma_device_stop(&p->device);
      p->state.store(State::Paused);
    }

    void Engine::stop() {
      Impl *p = d.get();
      if (p->state.load() == State::Stopped) return;
      p->stopDecodeThread();
      if (p->device_inited && ma_device_is_started(&p->device)) ma_device_stop(&p->device);
      if (p->rb_inited) ma_pcm_rb_reset(&p->rb);
      p->setFramesForMs(p->start_ms);
      p->state.store(State::Stopped);
    }

    void Engine::seek(int64_t abs_ms) {
      Impl *p = d.get();
      if (p->fmt == nullptr || !p->seekable) return;
      if (abs_ms < 0) abs_ms = 0;

      State prev = p->state.load();
      p->stopDecodeThread();
      if (p->device_inited && ma_device_is_started(&p->device)) ma_device_stop(&p->device);
      if (p->rb_inited) ma_pcm_rb_reset(&p->rb);
      p->doSeekFile(abs_ms);
      p->setFramesForMs(abs_ms);
      p->eof.store(false);
      if (prev != State::Stopped) p->startDecodeThread();
      if (prev == State::Playing && p->device_inited) ma_device_start(&p->device);
    }

    void Engine::setVolume(float g) {
      d->volume.store(g);
      if (d->device_inited) ma_device_set_master_volume(&d->device, g);
    }

    Engine::State Engine::state() const { return d->state.load(); }

    int64_t Engine::positionMs() const { return d->filePosMs(); }

    int64_t Engine::durationMs() const { return d->duration_ms; }

    bool Engine::isSeekable() const { return d->seekable; }

    bool Engine::setDevice(const void *pDeviceId) {
      return d->switchDevice(reinterpret_cast<const ma_device_id *>(pDeviceId));
    }

    std::vector<DeviceInfo> Engine::playbackDevices() {
      std::vector<DeviceInfo> out;
      if (!d->context_inited) return out;
      ma_device_info *infos = nullptr;
      ma_uint32 count = 0;
      if (ma_context_get_devices(&d->context, &infos, &count, nullptr, nullptr) != MA_SUCCESS) return out;
      out.reserve(count);
      for (ma_uint32 i = 0; i < count; ++i) {
        DeviceInfo di;
        di.id = d->serializeId(infos[i].id);
        di.name = infos[i].name;
        di.is_default = infos[i].isDefault != 0;
        out.push_back(std::move(di));
      }
      return out;
    }

    bool Engine::setDeviceById(const std::string &id) {
      if (id.empty()) return d->switchDevice(nullptr);   // system default
      if (!d->context_inited) return false;
      ma_device_info *infos = nullptr;
      ma_uint32 count = 0;
      if (ma_context_get_devices(&d->context, &infos, &count, nullptr, nullptr) != MA_SUCCESS) return false;
      for (ma_uint32 i = 0; i < count; ++i) {
        if (d->serializeId(infos[i].id) == id) {
          ma_device_id copy = infos[i].id;   // infos is owned by the context; copy before use
          return d->switchDevice(&copy);
        }
      }
      return false;   // a persisted device that is not currently present -> keep current
    }

  } // namespace Native
} // namespace Playback
