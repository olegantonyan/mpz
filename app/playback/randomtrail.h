#ifndef RANDOMTRAIL_H
#define RANDOMTRAIL_H

#include <QStack>

namespace Playback {
  class RandomTrail {
  public:
    explicit RandomTrail(int max_size = 64);

    void clear();

    void add(quint64 track_uid);
    bool exists(quint64 track_uid) const;
    quint64 prev();

  private:
    QStack<quint64> trail;
    int max_length;
  };
}

#endif // RANDOMTRAIL_H
