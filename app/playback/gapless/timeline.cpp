#include "playback/gapless/timeline.h"

namespace Playback::Gapless {
  qint64 Timeline::length(const Segment &s) {
    if (s.end_frame < 0) {
      return -1;
    }
    return qMax<qint64>(0, s.end_frame - s.begin_frame);
  }

  bool Timeline::valid(int idx) const {
    return idx >= 0 && idx < segments.size();
  }

  void Timeline::recomputeAbs() {
    qint64 acc = 0;
    for (Segment &s : segments) {
      s.abs_start = acc;
      const qint64 len = length(s);
      if (len < 0) {
        break;
      }
      acc += len;
    }
  }

  int Timeline::appendSegment(const Track &t, const QUrl &url, qint64 begin_frame, qint64 end_frame) {
    if (!segments.isEmpty() && length(segments.last()) < 0) {
      return -1;
    }
    Segment s;
    s.track = t;
    s.url = url;
    s.begin_frame = begin_frame;
    s.end_frame = end_frame;
    segments.append(s);
    recomputeAbs();
    return segments.size() - 1;
  }

  void Timeline::closeSegment(int idx, qint64 end_frame_in_file) {
    if (!valid(idx)) {
      return;
    }
    segments[idx].end_frame = qMax(end_frame_in_file, segments[idx].begin_frame);
    recomputeAbs();
  }

  void Timeline::removeLastSegment() {
    if (!segments.isEmpty()) {
      segments.removeLast();
      recomputeAbs();
    }
  }

  Timeline::Pos Timeline::map(qint64 absolute_frame) const {
    if (segments.isEmpty()) {
      return Pos();
    }
    const qint64 f = qMax<qint64>(0, absolute_frame);
    for (int i = 0; i < segments.size(); i++) {
      const Segment &s = segments.at(i);
      const qint64 len = length(s);
      if (len < 0) {
        if (f >= s.abs_start) {
          return Pos{i, s.track, s.begin_frame + (f - s.abs_start)};
        }
      } else if (f >= s.abs_start && f < s.abs_start + len) {
        return Pos{i, s.track, s.begin_frame + (f - s.abs_start)};
      }
    }
    return Pos();
  }

  qint64 Timeline::segmentEndAbs(int idx) const {
    if (!valid(idx)) {
      return -1;
    }
    const qint64 len = length(segments.at(idx));
    return len < 0 ? -1 : segments.at(idx).abs_start + len;
  }

  qint64 Timeline::segmentStartAbs(int idx) const {
    return valid(idx) ? segments.at(idx).abs_start : -1;
  }

  qint64 Timeline::absoluteForTrackMs(int idx, qint64 ms, int sample_rate) const {
    if (!valid(idx)) {
      return -1;
    }
    qint64 frames = qMax<qint64>(0, ms * sample_rate / 1000);
    const qint64 len = length(segments.at(idx));
    if (len >= 0 && frames > len) {
      frames = len;
    }
    return segments.at(idx).abs_start + frames;
  }

  int Timeline::segmentCount() const {
    return segments.size();
  }

  const Track &Timeline::segmentTrack(int idx) const {
    static const Track empty;
    return valid(idx) ? segments.at(idx).track : empty;
  }

  QUrl Timeline::segmentUrl(int idx) const {
    return valid(idx) ? segments.at(idx).url : QUrl();
  }

  qint64 Timeline::segmentBeginInFile(int idx) const {
    return valid(idx) ? segments.at(idx).begin_frame : 0;
  }

  void Timeline::reset(const Track &t, const QUrl &url, qint64 begin_frame, qint64 end_frame) {
    segments.clear();
    appendSegment(t, url, begin_frame, end_frame);
  }

  void Timeline::clear() {
    segments.clear();
  }
}
