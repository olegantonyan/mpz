#ifndef RNJESUS_H
#define RNJESUS_H

#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  #define USE_QRANDOMGENERATOR
#endif

class RNJesus {
public:
  static void seed();
  static quint64 generate(int max);
  static quint64 generate();
};

#endif // RNJESUS_H
