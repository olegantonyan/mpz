#ifndef RADIO_CATALOG_H
#define RADIO_CATALOG_H

#include "radio/station.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Radio {
  // Station list backing the radio library tree. Ships baked into the qrc and is
  // replaced wholesale by a user file when one is set and parses; a user file
  // that sets "extends_builtin" is layered over the built-in set by station id.
  // A malformed user file is rejected -- active() keeps serving the built-in
  // list and lastError() explains why.
  class Catalog {
  public:
    const QVector<Station> &stations() const;
    QStringList groups() const;
    const Station *byId(const QString &id) const;
    bool extendsBuiltin() const;

    static Catalog fromJson(const QByteArray &json, QString *error = nullptr);

    static const Catalog &active();
    static void reload();

    // "" disables the override. Set once at startup from
    // Config::Storage::configPath(); tests point it at a temp dir.
    static void setUserFilePath(const QString &path);
    static QString userFilePath();

    static QByteArray builtinJson();
    static QString lastError();

  private:
    void layerOver(const Catalog &base);

    QVector<Station> _stations;
    bool _extends_builtin = false;
  };
}

#endif // RADIO_CATALOG_H
