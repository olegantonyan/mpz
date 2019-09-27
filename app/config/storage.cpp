#include "storage.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QFile>

namespace Config {
  Storage::Storage(const QString &filepath_) {
    filepath = filepath_;
  }

  QString Storage::getString(const QString &key, bool *ok) {
    if (!QFile::exists(filepath)) {
      if (ok) {
        *ok = false;
      }
      return QString();
    }
    YAML::Node config = YAML::LoadFile(filepath.toStdString());
    YAML::Node result = config[key.toLatin1().data()];
    if (!result) {
      if (ok) {
        *ok = false;
      }
      return QString();
    }
    if (ok) {
      *ok = true;
    }
    return QString(result.as<std::string>().c_str());
  }

  int Storage::getInt(const QString &key, bool *ok) {
    if (!QFile::exists(filepath)) {
      if (ok) {
        *ok = false;
      }
      return -1;
    }
    YAML::Node config = YAML::LoadFile(filepath.toStdString());
    YAML::Node result = config[key.toLatin1().data()];
    if (!result) {
      if (ok) {
        *ok = false;
      }
      return -1;
    }
    if (ok) {
      *ok = true;
    }
    return result.as<int>();
  }
}
