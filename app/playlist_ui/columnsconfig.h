#ifndef COLUMNSCONFIG_H
#define COLUMNSCONFIG_H

#include "track.h"

#include <QString>

namespace PlaylistUi {
  class ColumnsConfig {
  public:
    explicit ColumnsConfig();

    int count() const;
    double width(int col) const;
    bool stretch(int col) const;
    QString field(int col) const;
    Qt::Alignment align(int col) const;

    QString value(int col, const Track& track) const;
  };
}

#endif // COLUMNSCONFIG_H
