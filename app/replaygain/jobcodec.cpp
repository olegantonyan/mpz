#include "replaygain/jobcodec.h"

#include <QDataStream>
#include <QIODevice>

namespace ReplayGain {
  // Must not be in an anonymous namespace: QList's streaming operator finds these by argument-dependent lookup, which never looks inside this file.
  QDataStream &operator<<(QDataStream &s, const Slice &slice) {
    return s << slice.begin_ms << slice.duration_ms;
  }

  QDataStream &operator>>(QDataStream &s, Slice &slice) {
    return s >> slice.begin_ms >> slice.duration_ms;
  }

  QDataStream &operator<<(QDataStream &s, const FileWork &work) {
    return s << work.path << work.slices;
  }

  QDataStream &operator>>(QDataStream &s, FileWork &work) {
    return s >> work.path >> work.slices;
  }

  QDataStream &operator<<(QDataStream &s, const Gain &gain) {
    return s << gain.track_db << gain.track_peak << gain.album_db << gain.album_peak
             << gain.has_track << gain.has_album;
  }

  QDataStream &operator>>(QDataStream &s, Gain &gain) {
    return s >> gain.track_db >> gain.track_peak >> gain.album_db >> gain.album_peak >>
           gain.has_track >> gain.has_album;
  }

  QDataStream &operator<<(QDataStream &s, const SliceResult &r) {
    return s << r.path << r.begin_ms << r.ok << r.error << r.gain << qint32(r.tag_result);
  }

  QDataStream &operator>>(QDataStream &s, SliceResult &r) {
    qint32 tag_result = -1;
    s >> r.path >> r.begin_ms >> r.ok >> r.error >> r.gain >> tag_result;
    r.tag_result = tag_result;
    return s;
  }

  namespace {
    const quint32 kVersion = 1;

    QByteArray frame(Message type, const QByteArray &payload) {
      QByteArray out;
      QDataStream s(&out, QIODevice::WriteOnly);
      s.setVersion(QDataStream::Qt_5_15);
      s << quint8(type) << payload;
      return out;
    }
  }

  QByteArray encodeJob(const Job &job) {
    QByteArray out;
    QDataStream s(&out, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_5_15);
    s << kVersion << qint32(job.epoch) << job.folder << job.files << job.want_album
      << job.write_tags;
    return out;
  }

  bool decodeJob(const QByteArray &data, Job &out) {
    QDataStream s(data);
    s.setVersion(QDataStream::Qt_5_15);
    quint32 version = 0;
    qint32 epoch = 0;
    s >> version;
    if (version != kVersion) {
      return false;
    }
    s >> epoch >> out.folder >> out.files >> out.want_album >> out.write_tags;
    out.epoch = epoch;
    return s.status() == QDataStream::Ok;
  }

  QByteArray frameFileStarted(const QString &path) {
    QByteArray payload;
    QDataStream s(&payload, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_5_15);
    s << path;
    return frame(Message::FileStarted, payload);
  }

  QByteArray frameJobDone(const JobResult &result) {
    QByteArray payload;
    QDataStream s(&payload, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_5_15);
    s << qint32(result.epoch) << result.folder << result.slices;
    return frame(Message::JobDone, payload);
  }

  bool takeMessage(QByteArray &buffer, Message &type, QByteArray &payload) {
    QDataStream s(buffer);
    s.setVersion(QDataStream::Qt_5_15);
    quint8 raw = 0;
    s >> raw;
    if (s.status() != QDataStream::Ok) {
      return false;
    }
    s >> payload;
    if (s.status() != QDataStream::Ok) {
      return false;
    }
    type = static_cast<Message>(raw);
    buffer.remove(0, int(s.device()->pos()));
    return true;
  }

  QString decodeFileStarted(const QByteArray &payload) {
    QDataStream s(payload);
    s.setVersion(QDataStream::Qt_5_15);
    QString path;
    s >> path;
    return path;
  }

  bool decodeJobDone(const QByteArray &payload, JobResult &out) {
    QDataStream s(payload);
    s.setVersion(QDataStream::Qt_5_15);
    qint32 epoch = 0;
    s >> epoch >> out.folder >> out.slices;
    out.epoch = epoch;
    return s.status() == QDataStream::Ok;
  }
}
