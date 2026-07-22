#ifndef EQ_BAND_H
#define EQ_BAND_H

namespace Eq {
  struct Band {
    enum class Type { Peaking, LowShelf, HighShelf, LowPass, HighPass };

    Type type = Type::Peaking;
    double freq_hz = 1000.0;
    double gain_db = 0.0;
    double q = 1.0;
    bool enabled = true;
  };
}

#endif // EQ_BAND_H
