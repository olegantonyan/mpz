#ifndef GAPLESS_TIMELINE_H
#define GAPLESS_TIMELINE_H

#include "track.h"

#include <QUrl>
#include <QVector>

namespace Playback::Gapless {
  class Timeline {
  public:
    struct Pos {
      int segment = -1;
      Track track;
      qint64 track_frame = 0;
    };

    int appendSegment(const Track &t, const QUrl &url, qint64 begin_frame, qint64 end_frame);
    void closeSegment(int idx, qint64 end_frame_in_file);
    void removeLastSegment();
    Pos map(qint64 absolute_frame) const;

    qint64 segmentEndAbs(int idx) const;
    qint64 segmentStartAbs(int idx) const;
    qint64 absoluteForTrackMs(int idx, qint64 ms, int sample_rate) const;

    int segmentCount() const;
    const Track &segmentTrack(int idx) const;
    QUrl segmentUrl(int idx) const;
    qint64 segmentBeginInFile(int idx) const;

    void reset(const Track &t, const QUrl &url, qint64 begin_frame, qint64 end_frame);
    void clear();

  private:
    struct Segment {
      Track track;
      QUrl url;
      qint64 begin_frame = 0;
      qint64 end_frame = -1;
      qint64 abs_start = 0;
    };

    static qint64 length(const Segment &s);
    void recomputeAbs();
    bool valid(int idx) const;

    QVector<Segment> segments;
  };
}

#endif // GAPLESS_TIMELINE_H
