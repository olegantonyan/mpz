#ifndef DYNAMIC_RANGE_DR_H
#define DYNAMIC_RANGE_DR_H

#include <QMetaType>
#include <QVector>

namespace DynamicRange {
  constexpr double MINUS_INF_DB = -1e9;

  struct Result {
    bool valid = false;
    double dr = 0.0;
    double peak_db = MINUS_INF_DB;
    double rms_db = MINUS_INF_DB;
    int channels = 0;
    int sample_rate = 0;
    qint64 frames = 0;
  };

  int displayDr(const Result &r);
  int officialAlbumDr(const QVector<Result> &results);

  class Accumulator {
  public:
    Accumulator(int channels, int sample_rate);

    void addInterleaved(const double *samples, qint64 frames);
    void setSampleRate(int rate);
    void invalidate();
    Result finish();

  private:
    struct Block {
      double rms = 0.0;
      double peak = 0.0;
    };

    struct Channel {
      double block_sq = 0.0;
      double block_peak = 0.0;
      double total_sq = 0.0;
      double total_peak = 0.0;
      QVector<Block> blocks;
    };

    void flushBlock();

    int channels;
    int sample_rate;
    qint64 block_frames;
    qint64 block_count = 0;
    qint64 total_frames = 0;
    bool broken = false;
    QVector<Channel> chans;
  };
}

Q_DECLARE_METATYPE(DynamicRange::Result)

#endif // DYNAMIC_RANGE_DR_H
