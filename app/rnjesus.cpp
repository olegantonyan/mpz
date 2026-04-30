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

quint64 RNJesus::generate(int size) {
  if (size <= 0) {
    return 0;
  }
#ifdef USE_QRANDOMGENERATOR
  return QRandomGenerator::global()->bounded(size);
#else
  return qrand() % size;
#endif
}

quint64 RNJesus::generate() {
#ifdef USE_QRANDOMGENERATOR
  return QRandomGenerator::global()->generate64();
#else
  return QDateTime::currentMSecsSinceEpoch() * (qrand() + 1) + (qrand() + 1);
#endif
}
