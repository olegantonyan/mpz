#ifndef RADIO_CATALOG_H
#define RADIO_CATALOG_H

#include "radio/station.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Radio {
  namespace Catalog {
    QByteArray builtinJson();
    QVector<Station> builtin();

    QVector<Station> fromJson(const QByteArray &json, QString *error = nullptr);

    QStringList groups(const QVector<Station> &stations);
  }
}

#endif // RADIO_CATALOG_H
