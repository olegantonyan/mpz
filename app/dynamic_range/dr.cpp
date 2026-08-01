// Implements the TT Dynamic Range Meter algorithm by the Pleasurize Music Foundation,
// as used by foobar2000's Dynamic Range Meter and dr14_tmeter.

#include "dynamic_range/dr.h"

#include <algorithm>
#include <cmath>

namespace DynamicRange {
  namespace {
    constexpr int BLOCK_SECONDS = 3;
    constexpr double TOP_FRACTION = 0.2;

    double toDb(double linear) {
      return linear > 0.0 ? 20.0 * std::log10(linear) : MINUS_INF_DB;
    }
  }

  int displayDr(const Result &r) {
    if (!r.valid) {
      return 0;
    }
    return qMax(0, int(std::lround(r.dr)));
  }

  int officialAlbumDr(const QVector<Result> &results) {
    int sum = 0;
    int count = 0;
    for (const auto &r : results) {
      if (!r.valid) {
        continue;
      }
      sum += displayDr(r);
      ++count;
    }
    if (count == 0) {
      return 0;
    }
    return int(std::lround(double(sum) / double(count)));
  }

  Accumulator::Accumulator(int ch, int rate) : channels(ch), sample_rate(rate) {
    block_frames = qMax(qint64(1), qint64(rate) * BLOCK_SECONDS);
    if (channels <= 0 || rate <= 0) {
      broken = true;
      return;
    }
    chans.resize(channels);
  }

  void Accumulator::setSampleRate(int rate) {
    if (rate <= 0 || rate == sample_rate) {
      return;
    }
    sample_rate = rate;
    block_frames = qMax(qint64(1), qint64(rate) * BLOCK_SECONDS);
  }

  void Accumulator::invalidate() {
    broken = true;
  }

  void Accumulator::addInterleaved(const double *samples, qint64 frames) {
    if (broken || samples == nullptr || frames <= 0) {
      return;
    }

    Channel *cd = chans.data();
    qint64 done = 0;
    while (done < frames) {
      const qint64 chunk = qMin(frames - done, block_frames - block_count);
      for (qint64 f = 0; f < chunk; ++f) {
        const double *frame = samples + (done + f) * channels;
        for (int c = 0; c < channels; ++c) {
          const double v = frame[c];
          const double a = std::fabs(v);
          cd[c].block_sq += v * v;
          cd[c].total_sq += v * v;
          if (a > cd[c].block_peak) {
            cd[c].block_peak = a;
          }
          if (a > cd[c].total_peak) {
            cd[c].total_peak = a;
          }
        }
      }
      done += chunk;
      block_count += chunk;
      total_frames += chunk;
      if (block_count >= block_frames) {
        flushBlock();
      }
    }
  }

  void Accumulator::flushBlock() {
    if (block_count <= 0) {
      return;
    }
    for (auto &c : chans) {
      Block b;
      b.rms = std::sqrt(2.0 * c.block_sq / double(block_count));
      b.peak = c.block_peak;
      c.blocks.append(b);
      c.block_sq = 0.0;
      c.block_peak = 0.0;
    }
    block_count = 0;
  }

  Result Accumulator::finish() {
    Result r;
    r.channels = channels;
    r.sample_rate = sample_rate;
    r.frames = total_frames;
    if (broken || channels <= 0 || total_frames <= 0) {
      return r;
    }

    // the trailing partial block is discarded, unless the track is shorter than one block
    if (chans.first().blocks.isEmpty()) {
      flushBlock();
    }
    const int blocks = chans.first().blocks.size();
    if (blocks <= 0) {
      return r;
    }
    const int top = qBound(1, int(std::floor(TOP_FRACTION * blocks + 0.5)), blocks);

    double peak_max = 0.0;
    double dr_sum = 0.0;
    double rms_db_sum = 0.0;
    int active = 0;
    QVector<double> rms;
    QVector<double> peaks;
    for (const auto &c : chans) {
      if (c.total_peak > peak_max) {
        peak_max = c.total_peak;
      }
      if (c.total_peak <= 0.0) {
        continue;
      }

      rms.clear();
      peaks.clear();
      rms.reserve(blocks);
      peaks.reserve(blocks);
      for (const auto &b : c.blocks) {
        rms.append(b.rms);
        peaks.append(b.peak);
      }
      std::sort(rms.begin(), rms.end(), std::greater<double>());
      std::sort(peaks.begin(), peaks.end(), std::greater<double>());

      double sq = 0.0;
      for (int i = 0; i < top; ++i) {
        sq += rms.at(i) * rms.at(i);
      }
      const double rms_top = std::sqrt(sq / double(top));
      const double second_peak = peaks.size() >= 2 ? peaks.at(1) : peaks.at(0);
      if (rms_top > 0.0 && second_peak > 0.0) {
        dr_sum += 20.0 * std::log10(second_peak / rms_top);
      }
      rms_db_sum += toDb(std::sqrt(2.0 * c.total_sq / double(total_frames)));
      ++active;
    }

    r.valid = true;
    if (active == 0) {
      return r;
    }
    r.dr = dr_sum / active;
    r.rms_db = rms_db_sum / active;
    r.peak_db = toDb(peak_max);
    return r;
  }
}
