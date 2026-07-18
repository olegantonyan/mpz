#include "playback/gapless/engine.h"

#include "playback/gapless/trackdecoder.h"

#include <QAudioDevice>
#include <QAudioSink>
#include <QIODevice>
#include <QLoggingCategory>
#include <QMediaDevices>

namespace Playback::Gapless {
  namespace {
    Q_LOGGING_CATEGORY(mpzGapless, "mpz.gapless", QtWarningMsg)
    bool sameFormat(const QAudioFormat &a, const QAudioFormat &b) {
      return a.sampleRate() == b.sampleRate() && a.channelCount() == b.channelCount() &&
             a.sampleFormat() == b.sampleFormat();
    }
  }

  Engine::Engine(qint64 cache_budget_bytes, QObject *parent)
      : QObject(parent), cache(cache_budget_bytes), cache_budget(cache_budget_bytes) {
    pump_timer.setInterval(50);
    connect(&pump_timer, &QTimer::timeout, this, &Engine::onPumpTick);
    position_timer.setInterval(100);
    connect(&position_timer, &QTimer::timeout, this, &Engine::emitPosition);
    connect(&media_devices, &QMediaDevices::audioOutputsChanged, this,
            [this]() { devices_changed_debounce.start(); });
    devices_changed_debounce.setSingleShot(true);
    devices_changed_debounce.setInterval(200); // audioOutputsChanged fires in bursts; coalesce into one evaluation
    connect(&devices_changed_debounce, &QTimer::timeout, this, &Engine::evaluateAudioDevice);
  }

  Engine::~Engine() = default;

  Playback::MediaPlayer::State Engine::state() const {
    return current_state;
  }

  qint64 Engine::positionMs() const {
    const int rate = sink_format.sampleRate();
    if (rate <= 0) {
      return 0;
    }
    const qint64 abs = audibleAbsFrame();
    const Timeline::Pos pos = timeline.map(abs);
    const int seg = pos.segment >= 0 ? pos.segment : current_segment;
    const qint64 start = timeline.segmentStartAbs(seg);
    const qint64 rel = start >= 0 ? qMax<qint64>(0, abs - start) : 0;
    return rel * 1000 / rate;
  }

  void Engine::setTrack(const Track &t) {
    const quint64 adopt_uid = boundary_adopt_uid;
    boundary_adopt_uid = 0;
    const int rate = sink_format.sampleRate();
    const bool live = current_state != MediaPlayer::StoppedState && sink && rate > 0;
    if (live && adopt_uid != 0 && t.uid() == adopt_uid) {
      const Timeline::Pos pos = timeline.map(audibleAbsFrame());
      if (pos.segment >= 0 && pos.track.uid() == t.uid()) {
        current_segment = pos.segment;
        synthetic_playing_on_play = true;
        qCDebug(mpzGapless) << "adopt boundary uid" << t.uid();
        return;
      }
    }
    if (live && t.isCue() && t.url() == current_url) {
      const qint64 begin_f = qint64(t.begin()) * rate / 1000;
      const qint64 dur_f = qint64(t.duration()) * rate / 1000;
      const Timeline::Pos pos = timeline.map(audibleAbsFrame());
      const bool inside = pos.segment >= 0 && pos.track_frame >= begin_f &&
                          (dur_f <= 0 || pos.track_frame < begin_f + dur_f);
      qCDebug(mpzGapless) << "cue same-file uid" << t.uid() << "inside" << inside;
      if (inside) {
        adoptRegion(t, pos);
        synthetic_playing_on_play = true;
        return;
      }
      resetPreparedState();
      qint64 end_f = dur_f > 0 ? begin_f + dur_f : -1;
      if (decoder_finished && (end_f < 0 || end_f > decoder_total_frames)) {
        end_f = decoder_total_frames;
      }
      timeline.reset(t, current_url, begin_f, end_f);
      current_track = t;
      current_segment = 0;
      about_to_finish_emitted = false;
      eof_done = false;
      setPositionMs(0);
      return;
    }
    qCDebug(mpzGapless) << "hard switch uid" << t.uid() << "cue" << t.isCue();
    hardSwitchTo(t);
  }

  void Engine::adoptRegion(const Track &t, const Timeline::Pos &pos) {
    const int rate = sink_format.sampleRate();
    const qint64 old_offset =
        timeline.segmentBeginInFile(pos.segment) - timeline.segmentStartAbs(pos.segment);
    const qint64 new_begin = qint64(t.begin()) * rate / 1000;
    qint64 new_end = t.duration() > 0 ? new_begin + qint64(t.duration()) * rate / 1000 : -1;
    if (decoder_finished && (new_end < 0 || new_end > decoder_total_frames)) {
      new_end = decoder_total_frames;
    }
    const qint64 delta = old_offset - new_begin;
    read_cursor_frame += delta;
    epoch_start_frame += delta;
    if (catchup_target_frame >= 0) {
      catchup_target_frame += delta;
    }
    timeline.reset(t, t.url(), new_begin, new_end);
    current_track = t;
    current_segment = 0;
    about_to_finish_emitted = false;
    eof_done = false;
  }

  void Engine::hardSwitchTo(const Track &t) {
    resetPreparedState();
    advance_on_eof = false;
    synthetic_playing_on_play = false;
    decoder_finished = false;
    decoder_total_frames = 0;
    eof_done = false;
    format_restart_done = false;
    about_to_finish_emitted = false;
    catchup_target_frame = -1;
    pending_seek_ms = -1;
    boundary_adopt_uid = 0;
    current_segment = 0;
    read_cursor_frame = 0;
    epoch_start_frame = 0;
    sink_format = QAudioFormat();
    if (sink) {
      sink->reset();
    }
    destroySink();
    cache.clear();
    timeline.clear();
    current_track = t;
    current_url = t.url();
    newDecoder();
    decoder->start(current_url);
  }

  void Engine::newDecoder() {
    if (decoder) {
      decoder->disconnect(this);
      decoder->deleteLater();
    }
    decoder = new TrackDecoder(this);
    connect(decoder, &TrackDecoder::firstFormat, this, &Engine::onFirstFormat);
    connect(decoder, &TrackDecoder::pcm, this, &Engine::onPcm);
    connect(decoder, &TrackDecoder::finished, this, &Engine::onDecoderFinished);
    connect(decoder, &TrackDecoder::decodeError, this, &Engine::onDecodeError);
  }

  void Engine::onFirstFormat(const QAudioFormat &format) {
    if (sink) {
      return; // already brought up (e.g. seek restart forces the sink format)
    }
    const QAudioDevice device = outputDevice();
    if (device.isFormatSupported(format) || format_restart_done) {
      setupPlayback(format);
      return;
    }
    format_restart_done = true;
    const QAudioFormat target = nearestSupported(device, format);
    if (decoder) {
      decoder->setPaused(true);
    }
    TrackDecoder *d = decoder;
    QTimer::singleShot(0, this, [this, d, target]() {
      if (decoder != d) {
        return;
      }
      decoder->stop();
      decoder->requestFormat(target);
      decoder->start(current_url);
    });
  }

  void Engine::setupPlayback(const QAudioFormat &format) {
    sink_format = format;
    const int rate = format.sampleRate();
    const qint64 begin_f = current_track.isCue() ? qint64(current_track.begin()) * rate / 1000 : 0;
    const qint64 end_f = (current_track.isCue() && current_track.duration() > 0)
                             ? begin_f + qint64(current_track.duration()) * rate / 1000
                             : -1;
    timeline.reset(current_track, current_url, begin_f, end_f);
    current_segment = 0;
    read_cursor_frame = 0;
    epoch_start_frame = 0;
    about_to_finish_emitted = false;
    cache.openEntry(current_url, format.bytesPerFrame());
    createSink();
    if (pending_seek_ms >= 0) {
      const qint64 ms = pending_seek_ms;
      pending_seek_ms = -1;
      setPositionMs(ms); // restore the pre-restart position after a forced format/device rebuild
    }
  }

  void Engine::createSink() {
    destroySink();
    active_device = outputDevice();
    sink = new QAudioSink(active_device, sink_format, this);
    sink->setBufferSize(sink_format.bytesForDuration(500000));
    sink->setVolume(volume_pct / 100.0);
    // queued: reset()/start() emit stateChanged synchronously, which must not re-enter feedSink mid-mutation
    connect(sink, &QAudioSink::stateChanged, this, [this](QAudio::State s) {
      if (s == QAudio::IdleState) {
        feedSink();
        checkEof();
      }
    }, Qt::QueuedConnection);
    sink_io = sink->start();
    epoch_start_frame = read_cursor_frame;
    if (current_state != MediaPlayer::PlayingState) {
      sink->suspend(); // brought up before play(): stay silent until play()
    }
  }

  void Engine::destroySink() {
    if (sink) {
      sink->stop();
      sink->deleteLater();
      sink = nullptr;
    }
    sink_io = nullptr;
  }

  void Engine::sinkEnsureStarted() {
    if (sink && sink->state() == QAudio::StoppedState) {
      sink_io = sink->start();
      epoch_start_frame = read_cursor_frame;
    }
  }

  void Engine::onPcm(const QUrl &url, const QByteArray &bytes) {
    if (url != current_url) {
      return;
    }
    cache.append(url, bytes.constData(), bytes.size());
    feedSink();
    applyBackPressure();
  }

  void Engine::onDecoderFinished(const QUrl &url, qint64 total_frames) {
    if (url != current_url) {
      return;
    }
    decoder_finished = true;
    decoder_total_frames = total_frames;
    for (int i = 0; i < timeline.segmentCount(); i++) {
      if (timeline.segmentUrl(i) != url) {
        continue;
      }
      const qint64 end_abs = timeline.segmentEndAbs(i);
      const qint64 end_in_file =
          end_abs < 0 ? -1 : timeline.segmentBeginInFile(i) + (end_abs - timeline.segmentStartAbs(i));
      if (end_in_file < 0 || end_in_file > total_frames) {
        timeline.closeSegment(i, total_frames); // VBR drift: clamp every region of this file to the real length
      }
    }
    if (prepared_append_deferred) {
      appendPreparedSegment();
    }
  }

  void Engine::onDecodeError(const QUrl &url, const QString &message) {
    if (url != current_url) {
      return;
    }
    emit error(message);
    decoder_finished = true;
    eof_done = true;
    finishPlayback();
  }

  void Engine::play() {
    if (synthetic_playing_on_play) {
      synthetic_playing_on_play = false;
      if (sink && sink->state() == QAudio::SuspendedState) {
        sink->resume();
      }
      setState(MediaPlayer::PlayingState);
      return;
    }
    if (sink && catchup_target_frame < 0) { // during seek catch-up the sink stays suspended; maybeFinishCatchUp resumes it at the target
      sinkEnsureStarted();
      if (sink->state() == QAudio::SuspendedState) {
        sink->resume();
      }
    }
    setState(MediaPlayer::PlayingState);
  }

  void Engine::pause() {
    if (current_state == MediaPlayer::PausedState) {
      play();
      return;
    }
    if (sink) {
      sink->suspend();
    }
    setState(MediaPlayer::PausedState);
  }

  void Engine::stop() {
    resetPreparedState();
    advance_on_eof = false;
    synthetic_playing_on_play = false;
    boundary_adopt_uid = 0;
    pending_seek_ms = -1;
    if (sink) {
      sink->reset();
      sink->stop();
      sink_io = nullptr;
    }
    if (decoder) {
      decoder->stop();
    }
    if (current_state != MediaPlayer::StoppedState) {
      setState(MediaPlayer::StoppedState); // Controller clears the track from its Stopped handler; a redundant emit here recurses
    }
  }

  void Engine::clearTrack() {
    stop();
    cache.clear();
    timeline.clear();
    destroySink();
    if (decoder) {
      decoder->deleteLater();
      decoder = nullptr;
    }
    current_url = QUrl();
    current_track = Track();
    current_segment = 0;
    read_cursor_frame = 0;
    epoch_start_frame = 0;
    catchup_target_frame = -1;
    pending_seek_ms = -1;
    boundary_adopt_uid = 0;
    decoder_finished = false;
    eof_done = false;
    about_to_finish_emitted = false;
    synthetic_playing_on_play = false;
    sink_format = QAudioFormat();
  }

  void Engine::setPositionMs(qint64 ms) {
    const int rate = sink_format.sampleRate();
    if (rate <= 0 || timeline.segmentCount() == 0) {
      return;
    }
    const qint64 target_abs = timeline.absoluteForTrackMs(current_segment, ms, rate);
    if (target_abs < 0) {
      return;
    }
    const qint64 target_in_file = inFileFrame(target_abs);
    const qint64 first = cache.firstFrame(current_url);
    const qint64 frontier = cache.frontierFrame(current_url);
    eof_done = false;
    if (target_in_file >= first && target_in_file < frontier) {
      read_cursor_frame = target_abs;
      epoch_start_frame = target_abs;
      catchup_target_frame = -1;
      if (sink) {
        sink_io = nullptr;
        sink->reset();
        sink_io = sink->start();
        if (current_state != MediaPlayer::PlayingState) {
          sink->suspend();
        }
      }
    } else if (target_in_file >= frontier) {
      catchUpTo(target_abs);
    } else {
      restartDecoderForSeek(target_abs);
    }
    emit positionChanged(ms);
  }

  void Engine::catchUpTo(qint64 target_abs) {
    // Sink stays silent; feeding resumes once the decode frontier reaches the target.
    catchup_target_frame = target_abs;
    read_cursor_frame = target_abs;
    if (decoder) {
      decoder->setPaused(false);
    }
    if (sink && sink->state() != QAudio::SuspendedState) {
      sink->suspend();
    }
  }

  void Engine::restartDecoderForSeek(qint64 target_abs) {
    // Target is behind the retained window; the decoder cannot seek, so re-open
    // the entry and discard-append from the file start until the target frame.
    cache.dropEntry(current_url);
    cache.openEntry(current_url, sink_format.bytesPerFrame());
    newDecoder();
    decoder->requestFormat(sink_format);
    decoder->start(current_url);
    decoder_finished = false;
    catchUpTo(target_abs);
  }

  void Engine::maybeFinishCatchUp() {
    const qint64 target_in_file = inFileFrame(catchup_target_frame);
    if (cache.frontierFrame(current_url) < target_in_file && !decoder_finished) {
      return;
    }
    read_cursor_frame = catchup_target_frame;
    epoch_start_frame = catchup_target_frame; // re-anchor the sink clock to the seek target
    catchup_target_frame = -1;
    if (sink) {
      sink_io = nullptr;
      sink->reset();
      sink_io = sink->start();
      if (current_state != MediaPlayer::PlayingState) {
        sink->suspend();
      }
    }
  }

  void Engine::setVolume(int pct) {
    volume_pct = pct;
    if (sink) {
      sink->setVolume(pct / 100.0);
    }
  }

  void Engine::prepareNextTrack(const Track &t) {
    resetPreparedState();
    if (t.uid() == 0 || current_state == MediaPlayer::StoppedState || sink_format.sampleRate() <= 0) {
      qCDebug(mpzGapless) << "prepare none" << t.uid() << "state" << current_state;
      return;
    }
    if (t.url() == current_url) {
      prepareSameFileContinuation(t);
    } else {
      prepareNewFile(t);
    }
  }

  void Engine::prepareSameFileContinuation(const Track &t) {
    const int rate = sink_format.sampleRate();
    const int last = timeline.segmentCount() - 1;
    if (last < 0 || timeline.segmentTrack(last).uid() == t.uid() || timeline.segmentEndAbs(last) < 0) {
      return;
    }
    const qint64 begin_f = qint64(t.begin()) * rate / 1000;
    const qint64 end_f = t.duration() > 0 ? begin_f + qint64(t.duration()) * rate / 1000 : -1;
    if (timeline.appendSegment(t, current_url, begin_f, end_f) < 0) {
      return;
    }
    prepared_url = current_url;
    prepared_track = t;
    prepared_begin_frame = begin_f;
    prepared_end_frame = end_f;
    prepared_segment_appended = true;
    qCDebug(mpzGapless) << "prepare same-file" << t.uid();
  }

  void Engine::prepareNewFile(const Track &t) {
    prepared_url = t.url();
    prepared_track = t;
    prebuffer_decoder = new TrackDecoder(this);
    connect(prebuffer_decoder, &TrackDecoder::firstFormat, this, &Engine::onPrebufferFirstFormat);
    connect(prebuffer_decoder, &TrackDecoder::pcm, this, &Engine::onPrebufferPcm);
    connect(prebuffer_decoder, &TrackDecoder::finished, this, &Engine::onPrebufferFinished);
    connect(prebuffer_decoder, &TrackDecoder::decodeError, this, &Engine::onPrebufferError);
    prebuffer_decoder->start(prepared_url);
  }

  void Engine::onPrebufferFirstFormat(const QAudioFormat &format) {
    const int rate = format.sampleRate();
    if (rate <= 0) {
      return;
    }
    prepared_format = format;
    prepared_begin_frame = prepared_track.isCue() ? qint64(prepared_track.begin()) * rate / 1000 : 0;
    prepared_end_frame = prepared_track.duration() > 0
                             ? prepared_begin_frame + qint64(prepared_track.duration()) * rate / 1000
                             : -1;
    const qint64 cap = qMin(cache_budget * 15 / 100, format.bytesForDuration(qint64(10) * 1000000));
    cache.setPrebufferEntry(prepared_url, format.bytesPerFrame(), cap);
    cache.protect(prepared_url, prepared_begin_frame); // pin the head so the capped ring holds the track start
    const bool continuous = sameFormat(format, sink_format);
    qCDebug(mpzGapless) << "prepare" << (continuous ? "continuous" : "drain-restart") << prepared_url;
    if (continuous) {
      if (decoder_finished) {
        appendPreparedSegment();
      } else {
        prepared_append_deferred = true; // continuous append waits until the current segment closes
      }
    } else {
      prepared_drain_restart = true;
    }
  }

  void Engine::onPrebufferPcm(const QUrl &url, const QByteArray &bytes) {
    if (url != prepared_url || !prebuffer_decoder) {
      return;
    }
    cache.append(url, bytes.constData(), bytes.size());
    feedSink();
    prebuffer_decoder->setPaused(!cache.wantsMoreData(prepared_url));
  }

  void Engine::onPrebufferFinished(const QUrl &url, qint64 total_frames) {
    if (url != prepared_url) {
      return;
    }
    prepared_decoder_finished = true;
    prepared_total_frames = total_frames;
    if (prepared_segment_appended) {
      const int idx = timeline.segmentCount() - 1;
      const qint64 end_abs = timeline.segmentEndAbs(idx);
      const qint64 end_in_file =
          end_abs < 0 ? -1 : timeline.segmentBeginInFile(idx) + (end_abs - timeline.segmentStartAbs(idx));
      if (end_in_file < 0 || end_in_file > total_frames) {
        timeline.closeSegment(idx, total_frames);
      }
    }
  }

  void Engine::onPrebufferError(const QUrl &url, const QString &) {
    if (url != prepared_url) {
      return;
    }
    resetPreparedState(); // next stays unprepared; the current file still advances via EOF
  }

  qint64 Engine::preparedEndFrame() const {
    qint64 end = prepared_end_frame;
    if (prepared_decoder_finished && (end < 0 || end > prepared_total_frames)) {
      end = prepared_total_frames;
    }
    return end;
  }

  void Engine::appendPreparedSegment() {
    if (prepared_segment_appended || prepared_url.isEmpty()) {
      return;
    }
    if (timeline.appendSegment(prepared_track, prepared_url, prepared_begin_frame, preparedEndFrame()) < 0) {
      prepared_append_deferred = true; // current segment still open; retry once it closes
      return;
    }
    prepared_segment_appended = true;
    prepared_append_deferred = false;
  }

  void Engine::performDrainRestart() {
    destroySink();
    const QAudioDevice device = outputDevice();
    QAudioFormat chosen = prepared_format;
    if (!device.isFormatSupported(chosen)) {
      chosen = nearestSupported(device, prepared_format);
    }
    const bool reformat = !sameFormat(chosen, prepared_format);
    qCDebug(mpzGapless) << "drain-restart" << prepared_url << "reformat" << reformat;
    cache.promote(prepared_url);
    if (decoder) {
      decoder->disconnect(this);
      decoder->deleteLater();
    }
    decoder = prebuffer_decoder;
    prebuffer_decoder = nullptr;
    decoder->disconnect(this);
    connect(decoder, &TrackDecoder::firstFormat, this, &Engine::onFirstFormat);
    connect(decoder, &TrackDecoder::pcm, this, &Engine::onPcm);
    connect(decoder, &TrackDecoder::finished, this, &Engine::onDecoderFinished);
    connect(decoder, &TrackDecoder::decodeError, this, &Engine::onDecodeError);
    sink_format = chosen;
    current_url = prepared_url;
    current_track = prepared_track;
    current_segment = 0;
    read_cursor_frame = 0;
    epoch_start_frame = 0;
    about_to_finish_emitted = false;
    eof_done = false;
    if (reformat) {
      // prepared native format unsupported on the device: re-decode into the supported
      // format (the prebuffered native PCM and native-rate frames are unusable)
      const int rate = chosen.sampleRate();
      const qint64 begin_f = current_track.isCue() ? qint64(current_track.begin()) * rate / 1000 : 0;
      const qint64 end_f = (current_track.isCue() && current_track.duration() > 0)
                               ? begin_f + qint64(current_track.duration()) * rate / 1000
                               : -1;
      cache.dropEntry(current_url);
      cache.openEntry(current_url, chosen.bytesPerFrame());
      timeline.reset(current_track, current_url, begin_f, end_f);
      decoder_finished = false;
      decoder_total_frames = 0;
      decoder->stop();
      decoder->requestFormat(chosen);
      decoder->start(current_url);
    } else {
      decoder_finished = prepared_decoder_finished;
      decoder_total_frames = prepared_total_frames;
      timeline.reset(prepared_track, prepared_url, prepared_begin_frame, preparedEndFrame());
    }
    clearPreparedFlags();
    boundary_adopt_uid = current_track.uid();
    createSink();
    emit positionChanged(0);
    QTimer::singleShot(0, this, [this]() { emit nextRequested(); });
  }

  void Engine::resetPreparedState() {
    if (prebuffer_decoder) {
      prebuffer_decoder->disconnect(this);
      prebuffer_decoder->deleteLater();
      prebuffer_decoder = nullptr;
    }
    if (!prepared_url.isEmpty() && prepared_url != current_url) {
      cache.dropEntry(prepared_url);
    }
    if (prepared_segment_appended && timeline.segmentCount() > current_segment + 1) {
      timeline.removeLastSegment();
    }
    clearPreparedFlags();
  }

  void Engine::clearPreparedFlags() {
    prepared_url = QUrl();
    prepared_track = Track();
    prepared_format = QAudioFormat();
    prepared_begin_frame = 0;
    prepared_end_frame = -1;
    prepared_total_frames = 0;
    prepared_segment_appended = false;
    prepared_append_deferred = false;
    prepared_drain_restart = false;
    prepared_decoder_finished = false;
  }

  void Engine::setOutputDevice(QByteArray id) {
    output_device_id = id;
    preferred_device_missing = false;
    ++device_change_epoch; // invalidate a pending debounced evaluation
    QAudioDevice target = findPreferredDevice();
    if (target.isNull()) {
      preferred_device_missing = !id.isEmpty();
      target = QMediaDevices::defaultAudioOutput();
    }
    switchSink(target);
  }

  QAudioDevice Engine::findPreferredDevice() const {
    if (output_device_id.isEmpty()) {
      return QAudioDevice();
    }
    const auto devices = QMediaDevices::audioOutputs();
    for (const auto &device : devices) {
      if (device.id() == output_device_id) {
        return device;
      }
    }
    return QAudioDevice();
  }

  void Engine::evaluateAudioDevice() {
    ++device_change_epoch;
    const QAudioDevice preferred = findPreferredDevice();
    QAudioDevice target;
    if (preferred.isNull()) {
      preferred_device_missing = !output_device_id.isEmpty();
      target = QMediaDevices::defaultAudioOutput();
      if (sink && active_device.id() == target.id()) {
        return; // already following the default
      }
    } else {
      if (!preferred_device_missing && sink && active_device.id() == preferred.id()) {
        return; // unrelated hotplug: don't disturb playback
      }
      preferred_device_missing = false;
      target = preferred;
    }
    const int epoch = device_change_epoch;
    QTimer::singleShot(0, this, [this, epoch, target]() {
      if (epoch != device_change_epoch) {
        return; // superseded by a newer device change
      }
      switchSink(target);
    });
  }

  void Engine::switchSink(const QAudioDevice &device) {
    if (!sink) {
      active_device = device; // stopped/no track: createSink re-resolves on the next start
      return;
    }
    if (device.id() == active_device.id()) {
      return;
    }
    qCDebug(mpzGapless) << "device switchSink" << active_device.id() << "->" << device.id();
    const qint64 captured_frame = audibleAbsFrame();
    const qint64 captured_ms = positionMs();
    sink->reset(); // discard buffered audio; do not drain
    read_cursor_frame = captured_frame;
    if (!device.isFormatSupported(sink_format)) {
      const QAudioFormat target = nearestSupported(device, sink_format);
      if (!sameFormat(target, sink_format)) {
        const Track t = current_track;
        hardSwitchTo(t); // rebuild pipeline; onFirstFormat picks the device's supported format at the new rate
        pending_seek_ms = captured_ms;
        return;
      }
    }
    createSink(); // rebuild on the new device; epoch re-anchored to the captured frame
    feedSink();
  }

  void Engine::onPumpTick() {
    if (catchup_target_frame >= 0) {
      maybeFinishCatchUp();
    }
    protectCache();
    feedSink();
    applyBackPressure();
    maybeEmitAboutToFinish();
    checkSegmentBoundary();
    checkEof();
  }

  void Engine::emitPosition() {
    emit positionChanged(positionMs());
  }

  void Engine::feedSink() {
    if (!sink || !sink_io || current_state != MediaPlayer::PlayingState || catchup_target_frame >= 0) {
      return;
    }
    const int bpf = sink_format.bytesPerFrame();
    if (bpf <= 0) {
      return;
    }
    qint64 free_bytes = sink->bytesFree();
    QByteArray chunk;
    while (free_bytes >= bpf) {
      const Timeline::Pos pos = timeline.map(read_cursor_frame);
      if (pos.segment < 0) {
        break;
      }
      qint64 want = free_bytes / bpf;
      const qint64 seg_end = timeline.segmentEndAbs(pos.segment);
      if (seg_end >= 0) {
        const qint64 room = seg_end - read_cursor_frame;
        if (room <= 0) {
          break;
        }
        want = qMin(want, room);
      }
      chunk.resize(static_cast<int>(want * bpf));
      const qint64 got = cache.read(timeline.segmentUrl(pos.segment), pos.track_frame, chunk.data(), want);
      if (got <= 0) {
        break;
      }
      const qint64 wrote = sink_io->write(chunk.constData(), got * bpf);
      if (wrote <= 0) {
        break;
      }
      read_cursor_frame += wrote / bpf;
      free_bytes -= wrote;
      if (wrote < got * bpf) {
        break;
      }
    }
  }

  void Engine::applyBackPressure() {
    maybeCloseCurrentAtFrontier();
    if (!decoder) {
      return;
    }
    if (catchup_target_frame >= 0) {
      decoder->setPaused(false);
      return;
    }
    decoder->setPaused(!cache.wantsMoreData(current_url));
  }

  qint64 Engine::currentFileNeededEnd() const {
    int last = -1;
    for (int i = 0; i < timeline.segmentCount(); i++) {
      if (timeline.segmentUrl(i) == current_url) {
        last = i;
      }
    }
    if (last < 0) {
      return -1;
    }
    const qint64 end_abs = timeline.segmentEndAbs(last);
    if (end_abs < 0) {
      return -1; // last region of this file still open: a real EOF is still required
    }
    return timeline.segmentBeginInFile(last) + (end_abs - timeline.segmentStartAbs(last));
  }

  void Engine::maybeCloseCurrentAtFrontier() {
    // CUE track ending mid-file with a cross-file next prepared: once the file is
    // decoded up to that track's end there is nothing more to wait for, so synthesize
    // EOF instead of letting back-pressure pause the decoder forever.
    if (decoder_finished || prepared_url.isEmpty() || prepared_url == current_url) {
      return;
    }
    const qint64 needed_end = currentFileNeededEnd();
    const qint64 frontier = cache.frontierFrame(current_url);
    if (needed_end < 0 || frontier < needed_end) {
      return;
    }
    if (decoder) {
      decoder->stop();
    }
    decoder_finished = true;
    decoder_total_frames = frontier;
    if (prepared_append_deferred) {
      appendPreparedSegment();
    }
    checkEof();
  }

  void Engine::protectCache() {
    if (current_url.isEmpty()) {
      return;
    }
    const int rate = sink_format.sampleRate();
    const qint64 reserve = rate > 0 ? qint64(30) * rate : 0;
    // during forward-seek catch-up the sink is frozen; anchor to the seek target so the
    // skipped span is evictable instead of pinned behind the frozen audible frame
    const qint64 anchor = catchup_target_frame >= 0 ? catchup_target_frame : audibleAbsFrame();
    cache.protect(current_url, inFileFrame(anchor) - reserve);
  }

  void Engine::maybeEmitAboutToFinish() {
    if (about_to_finish_emitted) {
      return;
    }
    const qint64 end = timeline.segmentEndAbs(current_segment);
    const int rate = sink_format.sampleRate();
    if (end < 0 || rate <= 0) {
      return;
    }
    const qint64 lead = qint64(10) * rate;
    const qint64 start = timeline.segmentStartAbs(current_segment);
    const qint64 threshold = (end - start <= lead) ? start : end - lead;
    if (audibleAbsFrame() >= threshold) {
      about_to_finish_emitted = true;
      emit aboutToFinish();
    }
  }

  void Engine::checkSegmentBoundary() {
    const qint64 end = timeline.segmentEndAbs(current_segment);
    if (end < 0 || current_segment + 1 >= timeline.segmentCount()) {
      return;
    }
    if (audibleAbsFrame() < end) {
      return;
    }
    const int next = current_segment + 1;
    const bool cross_file = timeline.segmentUrl(next) != current_url;
    if (cross_file && prebuffer_decoder) {
      cache.promote(timeline.segmentUrl(next));
      decoder->disconnect(this);
      decoder->deleteLater();
      decoder = prebuffer_decoder;
      prebuffer_decoder = nullptr;
      decoder->disconnect(this);
      connect(decoder, &TrackDecoder::firstFormat, this, &Engine::onFirstFormat);
      connect(decoder, &TrackDecoder::pcm, this, &Engine::onPcm);
      connect(decoder, &TrackDecoder::finished, this, &Engine::onDecoderFinished);
      connect(decoder, &TrackDecoder::decodeError, this, &Engine::onDecodeError);
      decoder_finished = prepared_decoder_finished;
      decoder_total_frames = prepared_total_frames;
      current_url = timeline.segmentUrl(next);
      current_track = timeline.segmentTrack(next);
    }
    current_segment = next;
    about_to_finish_emitted = false;
    boundary_adopt_uid = timeline.segmentTrack(next).uid();
    if (prepared_segment_appended && next == timeline.segmentCount() - 1) {
      clearPreparedFlags(); // the prepared segment/decoder/entry are now the current ones
    }
    qCDebug(mpzGapless) << "boundary crossing" << (cross_file ? "cross-file" : "same-file") << boundary_adopt_uid;
    emit positionChanged(0);
    QTimer::singleShot(0, this, [this]() { emit nextRequested(); });
  }

  void Engine::checkEof() {
    if (!decoder_finished || eof_done) {
      return;
    }
    if (current_segment + 1 < timeline.segmentCount()) {
      return;
    }
    const qint64 end = timeline.segmentEndAbs(current_segment);
    if (end < 0 || read_cursor_frame < end) {
      return;
    }
    if (sink) {
      if (sink->state() != QAudio::IdleState) {
        return;
      }
      if (audibleAbsFrame() < end) {
        return;
      }
    }
    if (prepared_drain_restart) {
      // defer out of the sink's own stateChanged callback so we never destroy the sink in-place
      if (!drain_restart_scheduled) {
        drain_restart_scheduled = true;
        QTimer::singleShot(0, this, [this]() {
          drain_restart_scheduled = false;
          if (prepared_drain_restart) {
            performDrainRestart();
          }
        });
      }
      return;
    }
    eof_done = true;
    finishPlayback();
  }

  void Engine::finishPlayback() {
    if (advance_on_eof) {
      qCDebug(mpzGapless) << "EOF fallback advance"; // sink stopped: incoming setTrack hard-starts (no adoption)
      emit positionChanged(0);
      QTimer::singleShot(0, this, [this]() { emit nextRequested(); });
    } else {
      setState(MediaPlayer::StoppedState);
    }
  }

  void Engine::setState(Playback::MediaPlayer::State s) {
    current_state = s;
    if (s == MediaPlayer::PlayingState) {
      advance_on_eof = true;
      pump_timer.start();
      position_timer.start();
    } else if (s == MediaPlayer::PausedState) {
      pump_timer.stop();
      position_timer.start();
    } else {
      pump_timer.stop();
      position_timer.stop();
    }
    emit stateChanged(s);
  }

  qint64 Engine::audibleAbsFrame() const {
    if (!sink) {
      return read_cursor_frame;
    }
    const int rate = sink_format.sampleRate();
    if (rate <= 0) {
      return epoch_start_frame;
    }
    qint64 abs = epoch_start_frame + sink->processedUSecs() * qint64(rate) / 1000000;
    if (abs > read_cursor_frame) {
      abs = read_cursor_frame;
    }
    return abs;
  }

  qint64 Engine::inFileFrame(qint64 abs_frame) const {
    const Timeline::Pos pos = timeline.map(abs_frame);
    return pos.segment >= 0 ? pos.track_frame : abs_frame;
  }

  QAudioDevice Engine::outputDevice() const {
    const QAudioDevice preferred = findPreferredDevice();
    return preferred.isNull() ? QMediaDevices::defaultAudioOutput() : preferred;
  }

  QAudioFormat Engine::nearestSupported(const QAudioDevice &device, const QAudioFormat &format) const {
    QAudioFormat candidate = format;
    candidate.setSampleFormat(QAudioFormat::Int16);
    if (device.isFormatSupported(candidate)) {
      return candidate;
    }
    candidate.setSampleFormat(QAudioFormat::Float);
    if (device.isFormatSupported(candidate)) {
      return candidate;
    }
    return device.preferredFormat();
  }
}
