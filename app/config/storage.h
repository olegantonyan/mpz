#ifndef STORAGE_H
#define STORAGE_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QVariant>

namespace Config {
  class Storage {
  public:
    Storage(const QString &path);

    QVariant get(const QString &key, bool *ok = nullptr);
    bool set(const QString &key, const QVariant &value);

    bool save();
    bool reload();

  private:
    QString filepath;
    QMap<QString, QVariant> data;
  };
}
#endif // STORAGE_H
