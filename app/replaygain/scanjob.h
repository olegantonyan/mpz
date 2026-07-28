#ifndef REPLAYGAIN_SCANJOB_H
#define REPLAYGAIN_SCANJOB_H

#include "replaygain/gain.h"

#include <QMetaType>
#include <QString>
#include <QVector>

namespace ReplayGain {
  struct Slice {
    quint64 begin_ms = 0;
    quint64 duration_ms = 0;
  };

  struct FileWork {
    QString path;
    QVector<Slice> slices;
  };

  struct Job {
    int epoch = 0;
    QString folder;
    QVector<FileWork> files;
    bool want_album = false;
    bool write_tags = false;

    int sliceCount() const {
      int n = 0;
      for (const auto &f : files) {
        n += f.slices.size();
      }
      return n;
    }
  };

  struct SliceResult {
    QString path;
    quint64 begin_ms = 0;
    bool ok = false;
    QString error;
    Gain gain;
    int tag_result = -1;
  };

  struct JobResult {
    int epoch = 0;
    QString folder;
    QVector<SliceResult> slices;
  };
}

Q_DECLARE_METATYPE(ReplayGain::Job)
Q_DECLARE_METATYPE(ReplayGain::SliceResult)
Q_DECLARE_METATYPE(ReplayGain::JobResult)

#endif // REPLAYGAIN_SCANJOB_H
