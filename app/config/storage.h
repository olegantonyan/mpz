#ifndef STORAGE_H
#define STORAGE_H

#include <QString>
#include <QVariant>

namespace Config {
  class Storage {
  public:
    Storage(const QString &filepath_);

    QString getString(const QString &key);

  private:
    QString filepath;
  };
}
#endif // STORAGE_H
