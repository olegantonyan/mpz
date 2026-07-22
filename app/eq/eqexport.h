#ifndef EQ_EQEXPORT_H
#define EQ_EQEXPORT_H

#include "eq/eqprofile.h"

#include <QString>

namespace Eq {
  QString exportParametricEq(const EqProfile &profile);
  QString exportGraphicEq(const EqProfile &profile, int fs);
}

#endif // EQ_EQEXPORT_H
