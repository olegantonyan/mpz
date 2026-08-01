#include "replaygain/jobrunner.h"

#include "replaygain/analyzer.h"
#include "replaygain/tags.h"

#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QEventLoop>
#include <QFileInfo>
#include <QTimer>
#include <QUrl>

#include <memory>
#include <vector>

namespace ReplayGain {
  namespace {
    const int kStallTimeoutMs = 30000;

    struct SliceState {
      qint64 start_frame = 0;
      qint64 end_frame = -1;
      std::unique_ptr<Analyzer> analyzer;
      bool failed = false;
    };

    qint64 framesForMs(quint64 ms, int sample_rate) {
      return static_cast<qint64>(ms) * sample_rate / 1000;
    }

    bool feed(Analyzer &a, QAudioFormat::SampleFormat format, const char *data, qint64 frames) {
      switch (format) {
        case QAudioFormat::Float:
          return a.addFloat(reinterpret_cast<const float *>(data), static_cast<std::size_t>(frames));
        case QAudioFormat::Int16:
          return a.addInt16(reinterpret_cast<const int16_t *>(data), static_cast<std::size_t>(frames));
        case QAudioFormat::Int32:
          return a.addInt32(reinterpret_cast<const int32_t *>(data), static_cast<std::size_t>(frames));
        case QAudioFormat::UInt8:
          return a.addUInt8(reinterpret_cast<const uint8_t *>(data), static_cast<std::size_t>(frames));
        default:
          return false;
      }
    }

    void feedBuffer(const QAudioBuffer &buffer, qint64 buffer_start_frame,
                    std::vector<SliceState> &states) {
      const QAudioFormat format = buffer.format();
      const qint64 frames = buffer.frameCount();
      const int bytes_per_frame = format.bytesPerFrame();
      const char *data = buffer.constData<char>();
      if (!data || bytes_per_frame <= 0) {
        return;
      }

      const qint64 buffer_end_frame = buffer_start_frame + frames;
      for (auto &s : states) {
        if (s.failed || !s.analyzer) {
          continue;
        }
        const qint64 slice_end = s.end_frame < 0 ? buffer_end_frame : s.end_frame;
        const qint64 from = std::max(s.start_frame, buffer_start_frame);
        const qint64 to = std::min(slice_end, buffer_end_frame);
        if (to <= from) {
          continue;
        }
        const char *at = data + (from - buffer_start_frame) * bytes_per_frame;
        if (!feed(*s.analyzer, format.sampleFormat(), at, to - from)) {
          s.failed = true;
        }
      }
    }
  }

  JobRunner::JobRunner(QObject *parent) : QObject(parent) {
  }

  void JobRunner::cancel() {
    cancelled = true;
    if (active_loop) {
      active_loop->quit();
    }
  }

  void JobRunner::run(const ReplayGain::Job &job) {
    cancelled = false;

    JobResult result;
    result.epoch = job.epoch;
    result.folder = job.folder;

    std::vector<std::vector<SliceState>> per_file;
    per_file.resize(job.files.size());

    for (int fi = 0; fi < job.files.size(); fi++) {
      const FileWork &work = job.files.at(fi);
      std::vector<SliceState> &states = per_file[fi];

      if (cancelled) {
        break;
      }
      emit fileStarted(job.epoch, work.path);

      QString file_error;
      if (!QFileInfo::exists(work.path)) {
        file_error = tr("file is gone");
      }

      if (file_error.isEmpty()) {
        QAudioDecoder decoder;
        QEventLoop loop;
        QTimer stall;
        QAudioFormat first_format;
        bool format_known = false;
        qint64 frames_seen = 0;

        active_loop = &loop;
        stall.setSingleShot(true);
        stall.setInterval(kStallTimeoutMs);

        connect(&stall, &QTimer::timeout, &loop, [&]() {
          file_error = tr("decoder stalled");
          loop.quit();
        });

        connect(&decoder, &QAudioDecoder::bufferReady, &loop, [&]() {
          stall.start();
          while (decoder.bufferAvailable()) {
            if (cancelled) {
              loop.quit();
              return;
            }
            const QAudioBuffer buffer = decoder.read();
            if (!buffer.isValid()) {
              break;
            }
            const QAudioFormat format = buffer.format();
            if (!format_known) {
              first_format = format;
              format_known = true;
              for (const auto &slice : work.slices) {
                SliceState s;
                s.start_frame = framesForMs(slice.begin_ms, format.sampleRate());
                s.end_frame = slice.duration_ms > 0
                                  ? s.start_frame + framesForMs(slice.duration_ms, format.sampleRate())
                                  : -1;
                s.analyzer = std::make_unique<Analyzer>(
                    static_cast<unsigned>(format.channelCount()),
                    static_cast<unsigned long>(format.sampleRate()));
                s.failed = !s.analyzer->isValid();
                states.push_back(std::move(s));
              }
            } else if (format.sampleRate() != first_format.sampleRate() ||
                       format.channelCount() != first_format.channelCount() ||
                       format.sampleFormat() != first_format.sampleFormat()) {
              file_error = tr("format changed mid-stream");
              loop.quit();
              return;
            }
            feedBuffer(buffer, frames_seen, states);
            frames_seen += buffer.frameCount();
          }
        });

        connect(&decoder, &QAudioDecoder::finished, &loop, &QEventLoop::quit);
        connect(&decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), &loop, [&]() {
          file_error = decoder.errorString();
          loop.quit();
        });

        decoder.setSource(QUrl::fromLocalFile(work.path));
        decoder.start();
        stall.start();
        loop.exec();
        decoder.stop();
        active_loop = nullptr;

        if (file_error.isEmpty() && !format_known) {
          file_error = tr("nothing decoded");
        }
      }

      if (!file_error.isEmpty()) {
        states.clear();
        for (const auto &slice : work.slices) {
          SliceResult r;
          r.path = work.path;
          r.begin_ms = slice.begin_ms;
          r.error = file_error;
          result.slices.append(r);
        }
        continue;
      }

      for (int si = 0; si < work.slices.size(); si++) {
        SliceResult r;
        r.path = work.path;
        r.begin_ms = work.slices.at(si).begin_ms;

        SliceState &s = states[static_cast<std::size_t>(si)];
        const double lufs = s.failed ? -HUGE_VAL : s.analyzer->integratedLufs();
        if (!std::isfinite(lufs)) {
          r.error = tr("no measurable loudness");
          s.failed = true;
        } else {
          r.ok = true;
          r.gain.track_db = gainDbFromLufs(lufs);
          r.gain.track_peak = s.analyzer->truePeak();
          r.gain.has_track = true;
        }
        result.slices.append(r);
      }
    }

    if (cancelled) {
      emit jobFinished(result);
      return;
    }

    if (job.want_album) {
      std::vector<Analyzer *> album;
      double album_peak = 0.0;
      for (auto &states : per_file) {
        for (auto &s : states) {
          if (!s.failed && s.analyzer) {
            album.push_back(s.analyzer.get());
          }
        }
      }
      for (const auto &r : result.slices) {
        if (r.ok) {
          album_peak = std::max(album_peak, r.gain.track_peak);
        }
      }

      const double album_lufs = Analyzer::albumLufs(album);
      if (std::isfinite(album_lufs)) {
        const double album_db = gainDbFromLufs(album_lufs);
        for (auto &r : result.slices) {
          if (r.ok) {
            r.gain.album_db = album_db;
            r.gain.album_peak = album_peak;
            r.gain.has_album = true;
          }
        }
      }
    }

    if (job.write_tags) {
      for (const auto &work : job.files) {
        const bool sliced = work.slices.size() > 1;
        for (auto &r : result.slices) {
          if (r.path != work.path || !r.ok) {
            continue;
          }
          r.tag_result = static_cast<int>(sliced ? TagResult::Unsupported
                                                 : writeTags(work.path, r.gain));
        }
      }
    }

    emit jobFinished(result);
  }
}
