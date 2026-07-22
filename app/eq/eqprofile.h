#ifndef EQ_EQPROFILE_H
#define EQ_EQPROFILE_H

#include "band.h"

#include <QString>
#include <QVector>

#include <vector>

namespace Eq {
  struct EqProfile {
    QString name;
    bool enabled = false;
    double preamp_db = 0.0;
    bool auto_preamp = true;
    QVector<Band> bands;
  };

  inline QString bandTypeToString(Band::Type type) {
    switch (type) {
      case Band::Type::Peaking:   return "PK";
      case Band::Type::LowShelf:  return "LS";
      case Band::Type::HighShelf: return "HS";
      case Band::Type::LowPass:   return "LP";
      case Band::Type::HighPass:  return "HP";
    }
    return "PK";
  }

  inline Band::Type bandTypeFromString(const QString &s) {
    if (s == "LS") return Band::Type::LowShelf;
    if (s == "HS") return Band::Type::HighShelf;
    if (s == "LP") return Band::Type::LowPass;
    if (s == "HP") return Band::Type::HighPass;
    return Band::Type::Peaking;
  }

  inline std::vector<Band> toStdBands(const QVector<Band> &bands) {
    return std::vector<Band>(bands.begin(), bands.end());
  }

  inline EqProfile defaultGraphicProfile() {
    const double freqs[] = {31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
    EqProfile p;
    p.name = "Flat";
    p.enabled = true;
    p.auto_preamp = true;
    for (int i = 0; i < 10; ++i) {
      Band b;
      b.freq_hz = freqs[i];
      b.gain_db = 0.0;
      b.q = 1.41;
      if (i == 0) {
        b.type = Band::Type::LowShelf;
      } else if (i == 9) {
        b.type = Band::Type::HighShelf;
      } else {
        b.type = Band::Type::Peaking;
      }
      p.bands.push_back(b);
    }
    return p;
  }
}

#endif // EQ_EQPROFILE_H
