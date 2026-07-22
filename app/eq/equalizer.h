#ifndef EQ_EQUALIZER_H
#define EQ_EQUALIZER_H

#include "band.h"
#include "biquad.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Eq {
  // Not thread-safe: the mpz engine drives it entirely on the GUI thread.
  class Equalizer {
  public:
    void setEnabled(bool on);
    void setPreampDb(double db);
    void setAutoPreamp(bool on);
    void setBands(const std::vector<Band> &bands);
    void setSampleRate(int fs);

    bool enabled() const { return enabled_; }
    double effectivePreampDb() const;
    double autoPreampDb() const;
    bool isIdentity() const;
    double magnitudeResponseDb(double freq_hz) const;

    void processFloat(float *interleaved, std::size_t frames, int channels);
    void processInt16(int16_t *interleaved, std::size_t frames, int channels);
    void processInt32(int32_t *interleaved, std::size_t frames, int channels);

    void reset();

  private:
    void rebuild();
    void ensureChannels(int channels);
    double nextTpdf();

    std::vector<Band> bands_;
    int fs_ = 44100;
    bool enabled_ = false;
    bool auto_preamp_ = true;
    double manual_preamp_db_ = 0.0;

    std::vector<Biquad> coeffs_;
    std::vector<std::vector<Biquad>> channel_state_;
    int channel_count_ = 0;
    double preamp_lin_ = 1.0;

    uint32_t rng_ = 0x9e3779b9u;
  };
}

#endif // EQ_EQUALIZER_H
