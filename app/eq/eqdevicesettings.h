#ifndef EQ_EQDEVICESETTINGS_H
#define EQ_EQDEVICESETTINGS_H

#include <QString>

namespace Eq {
  struct DeviceSettings {
    bool enabled = false;
    QString profile;
  };
}

#endif // EQ_EQDEVICESETTINGS_H
