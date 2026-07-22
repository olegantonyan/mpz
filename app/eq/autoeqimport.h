#ifndef EQ_AUTOEQIMPORT_H
#define EQ_AUTOEQIMPORT_H

#include "band.h"

#include <QString>

#include <vector>

namespace Eq {
  struct ParsedEq {
    bool preamp_present = false;
    double preamp_db = 0.0;
    std::vector<Band> bands;

    bool empty() const { return bands.empty() && !preamp_present; }
  };

  // EqualizerAPO / AutoEQ "ParametricEQ.txt":
  //   Preamp: -6.5 dB
  //   Filter 1: ON PK Fc 105 Hz Gain 5.5 dB Q 0.70
  ParsedEq parseParametricEq(const QString &text);
}

#endif // EQ_AUTOEQIMPORT_H
