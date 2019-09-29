#ifndef STORAGE_H
#define STORAGE_H

#include <QString>
#include <QList>
#include <QStringList>

namespace Config {
  class Storage {
  public:
    Storage(const QString &filepath_);

    QString getString(const QString &key, bool *ok = nullptr);
    bool setString(const QString &key, const QString &value);

    int getInt(const QString &key, bool *ok = nullptr);
    bool setInt(const QString &key, int value);

    QStringList getStringList(const QString &key, bool *ok = nullptr);
    bool setStringList(const QString &key, const QStringList &value);

  private:
    QString filepath;
  };
}
#endif // STORAGE_H
