#ifndef STORAGE_H
#define STORAGE_H

#include <QString>

namespace Config {
  class Storage {
  public:
    Storage(const QString &filepath_);

    QString getString(const QString &key, bool *ok = nullptr);
    int getInt(const QString &key, bool *ok = nullptr);

    bool setString(const QString &key, const QString &value);
    bool setInt(const QString &key, int value);

  private:
    QString filepath;
  };
}
#endif // STORAGE_H
