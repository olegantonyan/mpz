#include "output.h"

#include <QDebug>
#include <QDateTime>

namespace Audio {
  Output::Output(Decoder *d) {
    Q_ASSERT(decoder);
  }

  Output::~Output() {
  }
}


