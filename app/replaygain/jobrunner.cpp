#include "replaygain/jobrunner.h"

#include "replaygain/analyzer.h"
#include "replaygain/jobcodec.h"
#include "replaygain/tags.h"

#include <audioproperties.h>
#include <fileref.h>

#include <QLoggingCategory>

#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QEventLoop>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>
#include <QUrl>

#ifdef Q_OS_UNIX
  #include <unistd.h>
#endif

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace ReplayGain {
  namespace {
    Q_LOGGING_CATEGORY(mpzReplayGain, "mpz.replaygain", QtWarningMsg)

    const int kStallTimeoutMs = 30000;
    const int kAbortPollMs = 25;
    const int kWorkerStartMs = 10000;
    const int kWorkerPollMs = 100;
    const int kWorkerStopMs = 5000;
    const int kWorkerNiceness = 10;

    // The in-process path decodes on a LowestPriority thread; a child process would
    // otherwise inherit this process's normal priority and lose that.
    void lowerPriority(QProcess &worker) {
#if defined(Q_OS_UNIX)
      worker.setChildProcessModifier([]() { ::nice(kWorkerNiceness); });
#elif defined(Q_OS_WIN)
      worker.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= BELOW_NORMAL_PRIORITY_CLASS;
      });
#else
      Q_UNUSED(worker)
#endif
    }

    bool hasAudioStream(const QString &path) {
      const TagLib::FileRef file(path.toUtf8().constData());
      if (file.isNull()) {
        return false;
      }
      const TagLib::AudioProperties *props = file.audioProperties();
      return props != nullptr && props->channels() > 0 && props->sampleRate() > 0;
    }

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

  void JobRunner::setWorker(const QString &program, const QStringList &arguments) {
    worker_program = program;
    worker_arguments = arguments;
  }

  void JobRunner::run(const ReplayGain::Job &job) {
    if (!worker_program.isEmpty() && runIsolated(job)) {
      return;
    }
    runInProcess(job);
  }

  // A worker that dies took the file it last announced with it, so that one is dropped
  // and the rest go round again: k bad files cost k+1 passes over one folder.
  bool JobRunner::runIsolated(const ReplayGain::Job &job) {
    JobResult result;
    result.epoch = job.epoch;
    result.folder = job.folder;

    QVector<FileWork> pending = job.files;
    QVector<FileWork> crashed;
    bool worker_ran = false;

    while (!pending.isEmpty() && !job.aborted()) {
      Job pass = job;
      pass.files = pending;
      pass.want_album = job.want_album && crashed.isEmpty();

      QProcess worker;
      worker.setProgram(worker_program);
      worker.setArguments(worker_arguments);
      worker.setProcessChannelMode(QProcess::ForwardedErrorChannel);
      lowerPriority(worker);
      worker.start();
      if (!worker.waitForStarted(kWorkerStartMs)) {
        if (!worker_ran) {
          return false; // never usable: let the caller decode in this process
        }
        break;
      }
      worker_ran = true;
      worker.write(encodeJob(pass));
      worker.closeWriteChannel();

      QByteArray buffer;
      QString in_flight;
      JobResult pass_result;
      bool have_result = false;

      auto drain = [&]() {
        buffer.append(worker.readAllStandardOutput());
        Message type = Message::FileStarted;
        QByteArray payload;
        while (takeMessage(buffer, type, payload)) {
          if (type == Message::FileStarted) {
            in_flight = decodeFileStarted(payload);
            emit fileStarted(job.epoch, in_flight);
          } else if (type == Message::JobDone) {
            have_result = decodeJobDone(payload, pass_result);
          }
        }
      };

      while (worker.state() != QProcess::NotRunning && !job.aborted()) {
        if (worker.waitForReadyRead(kWorkerPollMs)) {
          drain();
        }
      }
      if (job.aborted()) {
        worker.kill();
      }
      worker.waitForFinished(kWorkerStopMs);
      drain();

      if (job.aborted() || have_result) {
        result.slices += pass_result.slices;
        break;
      }
      const auto blamed = std::find_if(pending.cbegin(), pending.cend(),
                                       [&](const FileWork &w) { return w.path == in_flight; });
      if (blamed == pending.cend()) {
        break; // died without naming a file: nothing to blame or retry
      }
      crashed.append(*blamed);
      pending.erase(std::remove_if(pending.begin(), pending.end(),
                                   [&](const FileWork &w) { return w.path == in_flight; }),
                    pending.end());
    }

    for (const auto &work : crashed) {
      for (const auto &slice : work.slices) {
        SliceResult r;
        r.path = work.path;
        r.begin_ms = slice.begin_ms;
        r.error = QStringLiteral("decoder crashed on this file");
        result.slices.append(r);
      }
    }

    emit jobFinished(result);
    return true;
  }

  void JobRunner::runInProcess(const ReplayGain::Job &job) {
    JobResult result;
    result.epoch = job.epoch;
    result.folder = job.folder;

    std::vector<std::vector<SliceState>> per_file;
    per_file.resize(job.files.size());
    std::vector<std::pair<qsizetype, qsizetype>> slice_range(job.files.size(), {0, 0});
    bool any_file_failed = false;

    for (int fi = 0; fi < job.files.size(); fi++) {
      const FileWork &work = job.files.at(fi);
      std::vector<SliceState> &states = per_file[fi];
      slice_range[fi] = {result.slices.size(), result.slices.size()};

      if (job.aborted()) {
        break;
      }
      emit fileStarted(job.epoch, work.path);

      QString file_error;
      if (!QFileInfo::exists(work.path)) {
        file_error = tr("file is gone");
      } else if (!hasAudioStream(work.path)) {
        // A stream that reports no channels makes Qt build an empty ffmpeg channel
        // layout, and the plugin then crashes inside swr_init instead of erroring out.
        file_error = tr("unreadable audio stream");
      }
      qCDebug(mpzReplayGain) << "decoding" << work.path << "error" << file_error;

      if (file_error.isEmpty()) {
        QAudioDecoder decoder;
        QEventLoop loop;
        QTimer stall;
        QTimer abort_poll;
        bool format_known = false;
        qint64 frames_seen = 0;

        stall.setSingleShot(true);
        stall.setInterval(kStallTimeoutMs);

        abort_poll.setInterval(kAbortPollMs);
        connect(&abort_poll, &QTimer::timeout, &loop, [&]() {
          if (job.aborted()) {
            loop.quit();
          }
        });

        connect(&stall, &QTimer::timeout, &loop, [&]() {
          file_error = tr("decoder stalled");
          loop.quit();
        });

        connect(&decoder, &QAudioDecoder::bufferReady, &loop, [&]() {
          stall.start();
          while (decoder.bufferAvailable()) {
            if (job.aborted()) {
              loop.quit();
              return;
            }
            const QAudioBuffer buffer = decoder.read();
            if (!buffer.isValid()) {
              break;
            }
            const QAudioFormat format = buffer.format();
            if (!format_known) {
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
        abort_poll.start();
        if (!job.aborted()) {
          loop.exec();
        }
        decoder.stop();

        if (file_error.isEmpty() && !format_known) {
          file_error = tr("nothing decoded");
        }
      }

      if (job.aborted()) {
        break; // an interrupted file is not a failed one
      }

      if (!file_error.isEmpty()) {
        any_file_failed = true;
        states.clear();
        for (const auto &slice : work.slices) {
          SliceResult r;
          r.path = work.path;
          r.begin_ms = slice.begin_ms;
          r.error = file_error;
          result.slices.append(r);
        }
        slice_range[fi].second = result.slices.size();
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
      slice_range[fi].second = result.slices.size();

      if (!job.want_album) {
        states.clear(); // ~1 MB each; only album gain needs them kept
      }
    }

    if (job.aborted()) {
      emit jobFinished(result);
      return;
    }

    // a gain measured over the surviving files is not this album's gain
    if (job.want_album && !any_file_failed) {
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
      for (int fi = 0; fi < job.files.size(); fi++) {
        if (job.aborted()) {
          break;
        }
        const FileWork &work = job.files.at(fi);
        const bool sliced = work.slices.size() > 1;
        for (qsizetype si = slice_range[fi].first; si < slice_range[fi].second; si++) {
          SliceResult &r = result.slices[si];
          if (!r.ok) {
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
