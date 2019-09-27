#ifndef STORAGE_H
#define STORAGE_H

#include <QString>

namespace Config {
  class Storage {
  public:
    Storage(const QString &filepath_);

    QString getString(const QString &key, bool *ok = nullptr);
    int getInt(const QString &key, bool *ok = nullptr);

  private:
    QString filepath;
  };
}
#endif // STORAGE_H
