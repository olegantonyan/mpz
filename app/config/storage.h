#ifndef STORAGE_H
#define STORAGE_H

#include "config/value.h"

#include <yaml-cpp/yaml.h>

#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <QByteArray>

namespace Config {
  class Storage {
  public:
    Storage(const QString &path);
    ~Storage();

    Config::Value get(const QString &key, bool *ok = nullptr) const;
    bool set(const QString &key, const Config::Value &value);

    QByteArray getByteArray(const QString &key, bool *ok = nullptr) const;
    bool set(const QString &key, const QByteArray &value);

    QList<int> getIntList(const QString &key, bool *ok = nullptr) const;
    bool set(const QString &key, const QList<int> &value);

    //QStringList getStringList(const QString &key, bool *ok = nullptr) const;
    //bool set(const QString &key, const QStringList &value);

    bool save();
    bool reload();

  private:
    QString filepath;
    QMap<QString, Config::Value> data;

    Config::Value castScalar(const QString& str) const;
    Config::Value castSequence(const QStringList& strl) const;

    bool changed;

    QMap<QString, Config::Value> parse(const YAML::Node &begin_node) const;
    YAML::Node serialize(const QMap<QString, Config::Value> &dt) const;
  };
}
#endif // STORAGE_H
