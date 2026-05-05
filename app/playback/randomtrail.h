#ifndef RANDOMTRAIL_H
#define RANDOMTRAIL_H

#include <QVector>

namespace Playback {
  class RandomTrail {
  public:
    explicit RandomTrail(int max_size = 64);

    void clear();

    void add(quint64 track_uid);
    bool recentlyPlayed(quint64 track_uid, int window) const;
    quint64 goPrev();

  private:
    QVector<quint64> trail;
    int cursor;
    int max_length;
  };
}

#endif // RANDOMTRAIL_H
