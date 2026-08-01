#include "replaygain/analyzer.h"

#include <ebur128.h>

namespace ReplayGain {
  namespace {
    const int kMode = EBUR128_MODE_I | EBUR128_MODE_SAMPLE_PEAK | EBUR128_MODE_TRUE_PEAK;
  }

  Analyzer::Analyzer(unsigned channels, unsigned long sample_rate) : channel_count(channels) {
    if (channels > 0 && sample_rate > 0) {
      state = ebur128_init(channels, sample_rate, kMode);
    }
  }

  Analyzer::~Analyzer() {
    if (state) {
      ebur128_destroy(&state);
    }
  }

  bool Analyzer::addFloat(const float *interleaved, std::size_t frames) {
    if (!state || !interleaved) {
      return false;
    }
    return ebur128_add_frames_float(state, interleaved, frames) == EBUR128_SUCCESS;
  }

  bool Analyzer::addInt16(const int16_t *interleaved, std::size_t frames) {
    if (!state || !interleaved) {
      return false;
    }
    return ebur128_add_frames_short(state, interleaved, frames) == EBUR128_SUCCESS;
  }

  bool Analyzer::addInt32(const int32_t *interleaved, std::size_t frames) {
    if (!state || !interleaved) {
      return false;
    }
    return ebur128_add_frames_int(state, interleaved, frames) == EBUR128_SUCCESS;
  }

  bool Analyzer::addUInt8(const uint8_t *interleaved, std::size_t frames) {
    if (!state || !interleaved) {
      return false;
    }
    const std::size_t samples = frames * channel_count;
    scratch.resize(samples);
    for (std::size_t i = 0; i < samples; i++) {
      scratch[i] = (static_cast<float>(interleaved[i]) - 128.0f) / 128.0f;
    }
    return addFloat(scratch.data(), frames);
  }

  double Analyzer::integratedLufs() const {
    double out = -HUGE_VAL;
    if (!state || ebur128_loudness_global(state, &out) != EBUR128_SUCCESS) {
      return -HUGE_VAL;
    }
    return out;
  }

  double Analyzer::truePeak() const {
    if (!state) {
      return 0.0;
    }
    double peak = 0.0;
    for (unsigned c = 0; c < channel_count; c++) {
      double v = 0.0;
      if (ebur128_true_peak(state, c, &v) == EBUR128_SUCCESS) {
        peak = std::max(peak, v);
      }
    }
    return peak;
  }

  double Analyzer::albumLufs(const std::vector<Analyzer *> &analyzers) {
    std::vector<ebur128_state *> states;
    states.reserve(analyzers.size());
    for (auto *a : analyzers) {
      if (a && a->state) {
        states.push_back(a->state);
      }
    }
    if (states.empty()) {
      return -HUGE_VAL;
    }
    double out = -HUGE_VAL;
    if (ebur128_loudness_global_multiple(states.data(), states.size(), &out) != EBUR128_SUCCESS) {
      return -HUGE_VAL;
    }
    return out;
  }
}
