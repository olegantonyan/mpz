#include "eq/equalizer.h"

#include <algorithm>
#include <cmath>

namespace Eq {
  namespace {
    constexpr double kMinDb = 1e-6;

    inline double dbToLin(double db) { return std::pow(10.0, db / 20.0); }

    inline int16_t clampInt16(double v) {
      if (v > 32767.0) return 32767;
      if (v < -32768.0) return -32768;
      return static_cast<int16_t>(std::lround(v));
    }

    inline int32_t clampInt32(double v) {
      if (v > 2147483647.0) return 2147483647;
      if (v < -2147483648.0) return -2147483648;
      return static_cast<int32_t>(std::llround(v));
    }
  }

  void Equalizer::setEnabled(bool on) {
    enabled_ = on;
  }

  void Equalizer::setPreampDb(double db) {
    manual_preamp_db_ = db;
    rebuild();
  }

  void Equalizer::setAutoPreamp(bool on) {
    auto_preamp_ = on;
    rebuild();
  }

  void Equalizer::setBands(const std::vector<Band> &bands) {
    bands_ = bands;
    rebuild();
  }

  void Equalizer::setSampleRate(int fs) {
    if (fs > 0 && fs != fs_) {
      fs_ = fs;
      rebuild();
    }
  }

  double Equalizer::autoPreampDb() const {
    double peak_db = 0.0;
    for (double f = 20.0; f <= 20000.0; f *= 1.0594630943592953) {
      const double w = 2.0 * kPi * f / fs_;
      if (w >= kPi) {
        break;
      }
      double db = 0.0;
      for (const auto &s : coeffs_) {
        const double m = s.magnitude(w);
        if (m > 0.0) {
          db += 20.0 * std::log10(m);
        }
      }
      peak_db = std::max(peak_db, db);
    }
    return -peak_db;
  }

  double Equalizer::effectivePreampDb() const {
    return auto_preamp_ ? autoPreampDb() : manual_preamp_db_;
  }

  void Equalizer::rebuild() {
    coeffs_.clear();
    for (const auto &b : bands_) {
      if (!b.enabled) {
        continue;
      }
      if ((b.type == Band::Type::Peaking || b.type == Band::Type::LowShelf ||
           b.type == Band::Type::HighShelf) &&
          std::fabs(b.gain_db) < kMinDb) {
        continue;
      }
      coeffs_.push_back(designBiquad(b, fs_));
    }
    preamp_lin_ = dbToLin(effectivePreampDb());
    channel_state_.clear();
  }

  void Equalizer::ensureChannels(int channels) {
    if (channels == channel_count_ && channel_state_.size() == static_cast<std::size_t>(channels)) {
      return;
    }
    channel_count_ = channels;
    channel_state_.assign(static_cast<std::size_t>(std::max(0, channels)), coeffs_);
  }

  bool Equalizer::isIdentity() const {
    if (!enabled_) {
      return true;
    }
    if (!coeffs_.empty()) {
      return false;
    }
    return std::fabs(effectivePreampDb()) < kMinDb;
  }

  double Equalizer::magnitudeResponseDb(double freq_hz) const {
    const double w = 2.0 * kPi * freq_hz / fs_;
    double db = effectivePreampDb();
    if (w > 0.0 && w < kPi) {
      for (const auto &s : coeffs_) {
        const double m = s.magnitude(w);
        if (m > 0.0) {
          db += 20.0 * std::log10(m);
        }
      }
    }
    return db;
  }

  double Equalizer::nextTpdf() {
    auto uniform = [this]() {
      rng_ ^= rng_ << 13;
      rng_ ^= rng_ >> 17;
      rng_ ^= rng_ << 5;
      return rng_ / 4294967296.0;
    };
    return uniform() - uniform();
  }

  void Equalizer::processFloat(float *data, std::size_t frames, int channels) {
    if (isIdentity() || channels <= 0) {
      return;
    }
    ensureChannels(channels);
    for (std::size_t i = 0; i < frames; ++i) {
      for (int c = 0; c < channels; ++c) {
        double x = static_cast<double>(data[i * channels + c]) * preamp_lin_;
        for (auto &s : channel_state_[c]) {
          x = s.process(x);
        }
        data[i * channels + c] = static_cast<float>(x);
      }
    }
  }

  void Equalizer::processInt16(int16_t *data, std::size_t frames, int channels) {
    if (isIdentity() || channels <= 0) {
      return;
    }
    ensureChannels(channels);
    for (std::size_t i = 0; i < frames; ++i) {
      for (int c = 0; c < channels; ++c) {
        double x = (static_cast<double>(data[i * channels + c]) / 32768.0) * preamp_lin_;
        for (auto &s : channel_state_[c]) {
          x = s.process(x);
        }
        data[i * channels + c] = clampInt16(x * 32768.0 + nextTpdf());
      }
    }
  }

  void Equalizer::processInt32(int32_t *data, std::size_t frames, int channels) {
    if (isIdentity() || channels <= 0) {
      return;
    }
    ensureChannels(channels);
    for (std::size_t i = 0; i < frames; ++i) {
      for (int c = 0; c < channels; ++c) {
        double x = (static_cast<double>(data[i * channels + c]) / 2147483648.0) * preamp_lin_;
        for (auto &s : channel_state_[c]) {
          x = s.process(x);
        }
        data[i * channels + c] = clampInt32(x * 2147483648.0);
      }
    }
  }

  void Equalizer::reset() {
    for (auto &ch : channel_state_) {
      for (auto &s : ch) {
        s.reset();
      }
    }
  }
}
