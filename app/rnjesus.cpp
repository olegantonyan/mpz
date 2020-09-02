#include "rnjesus.h"

#include <QDebug>

#ifdef USE_QRANDOMGENERATOR
  #include <QRandomGenerator>
#else
  #include <QDateTime>
#endif

void RNJesus::seed() {
#ifndef USE_QRANDOMGENERATOR
  qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);
#endif
}

quint64 RNJesus::generate(int max) {
#ifdef USE_QRANDOMGENERATOR
  return QRandomGenerator::global()->bounded(max);
#else
  int min = 0;
  return qrand() % ((max + 1) - min) + min;
#endif
}

quint64 RNJesus::generate() {
#ifdef USE_QRANDOMGENERATOR
  return QRandomGenerator::global()->generate64();
#else
  return QDateTime::currentMSecsSinceEpoch() * (qrand() + 1) + (qrand() + 1);
#endif
}
