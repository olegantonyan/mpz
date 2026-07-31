#ifndef PEAKS_H
#define PEAKS_H

#include <QByteArray>
#include <QVector>

namespace Waveform {
  struct Peaks {
    quint16 bucket_ms = 0;
    quint64 duration_ms = 0;
    QVector<quint8> peak;
    QVector<quint8> rms;

    bool isEmpty() const;
    QByteArray serialize() const;
    static Peaks deserialize(const QByteArray &blob);
  };
}

#endif // PEAKS_H
