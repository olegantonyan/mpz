#ifndef EQ_BIQUAD_H
#define EQ_BIQUAD_H

#include "eq/band.h"

#include <cmath>
#include <complex>

namespace Eq {
  struct Biquad {
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;

    inline double process(double x) {
      const double y = b0 * x + z1;
      z1 = b1 * x - a1 * y + z2;
      z2 = b2 * x - a2 * y;
      return y;
    }

    void reset() { z1 = z2 = 0.0; }

    double magnitude(double w) const {
      const std::complex<double> z1c = std::polar(1.0, -w);
      const std::complex<double> z2c = std::polar(1.0, -2.0 * w);
      const std::complex<double> num = b0 + b1 * z1c + b2 * z2c;
      const std::complex<double> den = std::complex<double>(1.0, 0.0) + a1 * z1c + a2 * z2c;
      return std::abs(num / den);
    }
  };

  // Robert Bristow-Johnson "Audio EQ Cookbook" formulas, matching EqualizerAPO.
  inline Biquad designBiquad(const Band &band, double fs) {
    Biquad bq;
    if (fs <= 0.0 || band.freq_hz <= 0.0) {
      return bq;
    }

    const double q = band.q > 1e-6 ? band.q : 1e-6;
    const double w0 = 2.0 * M_PI * std::min(band.freq_hz, fs * 0.5 * 0.999) / fs;
    const double cw = std::cos(w0);
    const double sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);

    double b0, b1, b2, a0, a1, a2;

    switch (band.type) {
      case Band::Type::Peaking: {
        const double A = std::pow(10.0, band.gain_db / 40.0);
        b0 = 1.0 + alpha * A;
        b1 = -2.0 * cw;
        b2 = 1.0 - alpha * A;
        a0 = 1.0 + alpha / A;
        a1 = -2.0 * cw;
        a2 = 1.0 - alpha / A;
        break;
      }
      case Band::Type::LowShelf: {
        const double A = std::pow(10.0, band.gain_db / 40.0);
        const double sqrtA2alpha = 2.0 * std::sqrt(A) * alpha;
        b0 = A * ((A + 1.0) - (A - 1.0) * cw + sqrtA2alpha);
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cw);
        b2 = A * ((A + 1.0) - (A - 1.0) * cw - sqrtA2alpha);
        a0 = (A + 1.0) + (A - 1.0) * cw + sqrtA2alpha;
        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cw);
        a2 = (A + 1.0) + (A - 1.0) * cw - sqrtA2alpha;
        break;
      }
      case Band::Type::HighShelf: {
        const double A = std::pow(10.0, band.gain_db / 40.0);
        const double sqrtA2alpha = 2.0 * std::sqrt(A) * alpha;
        b0 = A * ((A + 1.0) + (A - 1.0) * cw + sqrtA2alpha);
        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw);
        b2 = A * ((A + 1.0) + (A - 1.0) * cw - sqrtA2alpha);
        a0 = (A + 1.0) - (A - 1.0) * cw + sqrtA2alpha;
        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cw);
        a2 = (A + 1.0) - (A - 1.0) * cw - sqrtA2alpha;
        break;
      }
      case Band::Type::LowPass: {
        b0 = (1.0 - cw) / 2.0;
        b1 = 1.0 - cw;
        b2 = (1.0 - cw) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cw;
        a2 = 1.0 - alpha;
        break;
      }
      case Band::Type::HighPass: {
        b0 = (1.0 + cw) / 2.0;
        b1 = -(1.0 + cw);
        b2 = (1.0 + cw) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cw;
        a2 = 1.0 - alpha;
        break;
      }
      default:
        return bq;
    }

    bq.b0 = b0 / a0;
    bq.b1 = b1 / a0;
    bq.b2 = b2 / a0;
    bq.a1 = a1 / a0;
    bq.a2 = a2 / a0;
    return bq;
  }
}

#endif // EQ_BIQUAD_H
