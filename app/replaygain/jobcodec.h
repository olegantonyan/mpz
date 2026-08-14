#ifndef REPLAYGAIN_JOBCODEC_H
#define REPLAYGAIN_JOBCODEC_H

#include "replaygain/scanjob.h"

#include <QByteArray>
#include <QString>

namespace ReplayGain {
  // Wire format for the decode worker process.
  enum class Message : quint8 {
    FileStarted = 0,
    JobDone = 1,
  };

  QByteArray encodeJob(const Job &job);
  bool decodeJob(const QByteArray &data, Job &out);

  QByteArray frameFileStarted(const QString &path);
  QByteArray frameJobDone(const JobResult &result);

  // Pops one message off the front; false when the buffer holds only part of one.
  bool takeMessage(QByteArray &buffer, Message &type, QByteArray &payload);

  QString decodeFileStarted(const QByteArray &payload);
  bool decodeJobDone(const QByteArray &payload, JobResult &out);
}

#endif // REPLAYGAIN_JOBCODEC_H
