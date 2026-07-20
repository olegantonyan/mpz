#ifndef RADIO_CATALOG_H
#define RADIO_CATALOG_H

#include "radio/station.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Radio {
  // The baked-in station list shipped in the qrc. It seeds the user's list in
  // global.yml on first run and backs "Restore defaults"; after that the live
  // list lives in the config (see Config::Global::radioStations).
  namespace Catalog {
    QByteArray builtinJson();
    QVector<Station> builtin();

    // Parses and validates a station list. Rejects invalid JSON, a missing
    // "stations" array, a station without id/name, a duplicate id, or a url
    // that is not http/https.
    QVector<Station> fromJson(const QByteArray &json, QString *error = nullptr);

    // Groups in first-seen order, ungrouped stations excluded.
    QStringList groups(const QVector<Station> &stations);
  }
}

#endif // RADIO_CATALOG_H
