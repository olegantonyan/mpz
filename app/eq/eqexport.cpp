#include "eq/eqexport.h"

#include "eq/equalizer.h"

#include <cmath>

namespace Eq {
  namespace {
    QString paramToken(Band::Type t) {
      switch (t) {
        case Band::Type::Peaking:   return "PK";
        case Band::Type::LowShelf:  return "LSC";
        case Band::Type::HighShelf: return "HSC";
        case Band::Type::LowPass:   return "LPQ";
        case Band::Type::HighPass:  return "HPQ";
      }
      return "PK";
    }

    bool isPass(Band::Type t) {
      return t == Band::Type::LowPass || t == Band::Type::HighPass;
    }

    double effectivePreamp(const EqProfile &profile) {
      if (!profile.auto_preamp) {
        return profile.preamp_db;
      }
      Equalizer eq;
      eq.setSampleRate(48000);
      eq.setBands(toStdBands(profile.bands));
      return eq.autoPreampDb();
    }
  }

  QString exportParametricEq(const EqProfile &profile) {
    QString out;
    out += QString("Preamp: %1 dB\n").arg(effectivePreamp(profile), 0, 'f', 1);

    int n = 1;
    for (const auto &b : profile.bands) {
      const QString state = b.enabled ? "ON" : "OFF";
      const QString freq = QString::number(b.freq_hz, 'g', 6);
      const QString q = QString::number(b.q, 'g', 4);
      if (isPass(b.type)) {
        out += QString("Filter %1: %2 %3 Fc %4 Hz Q %5\n")
                 .arg(n).arg(state).arg(paramToken(b.type)).arg(freq).arg(q);
      } else {
        out += QString("Filter %1: %2 %3 Fc %4 Hz Gain %5 dB Q %6\n")
                 .arg(n).arg(state).arg(paramToken(b.type)).arg(freq)
                 .arg(QString::number(b.gain_db, 'f', 1)).arg(q);
      }
      ++n;
    }
    return out;
  }

  QString exportGraphicEq(const EqProfile &profile, int fs) {
    Equalizer eq;
    eq.setSampleRate(fs > 0 ? fs : 48000);
    eq.setAutoPreamp(profile.auto_preamp);
    eq.setBands(toStdBands(profile.bands));
    eq.setPreampDb(profile.preamp_db);

    QString points;
    int last_hz = -1;
    for (double f = 20.0; f <= 20000.0 + 1e-6; f *= 1.0594630943592953) {
      const int hz = static_cast<int>(std::lround(f));
      if (hz == last_hz) {
        continue;
      }
      last_hz = hz;
      const double db = eq.magnitudeResponseDb(hz);
      if (!points.isEmpty()) {
        points += "; ";
      }
      points += QString("%1 %2").arg(hz).arg(QString::number(db, 'f', 1));
    }
    return "GraphicEQ: " + points + "\n";
  }
}
