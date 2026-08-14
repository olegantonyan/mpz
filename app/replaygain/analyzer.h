#ifndef REPLAYGAIN_ANALYZER_H
#define REPLAYGAIN_ANALYZER_H

#include "replaygain/gain.h"

#include <ebur128.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ReplayGain {
  class Analyzer {
  public:
    Analyzer(unsigned channels, unsigned long sample_rate);
    ~Analyzer();

    Analyzer(const Analyzer &) = delete;
    Analyzer &operator=(const Analyzer &) = delete;

    bool isValid() const { return state != nullptr; }

    bool addFloat(const float *interleaved, std::size_t frames);
    bool addInt16(const int16_t *interleaved, std::size_t frames);
    bool addInt32(const int32_t *interleaved, std::size_t frames);
    bool addUInt8(const uint8_t *interleaved, std::size_t frames);

    double integratedLufs() const;
    double truePeak() const;

    static double albumLufs(const std::vector<Analyzer *> &analyzers);

  private:
    ebur128_state *state = nullptr;
    unsigned channel_count = 0;
    std::vector<float> scratch;
  };
}

#endif // REPLAYGAIN_ANALYZER_H
