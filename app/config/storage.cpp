#include "storage.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QFile>

namespace Config {
  Storage::Storage(const QString &filepath_) {
    filepath = filepath_;
  }

  QString Storage::getString(const QString &key) {
    if (!QFile::exists(filepath)) {
      return QString();
    }
    YAML::Node config = YAML::LoadFile(filepath.toStdString());
    YAML::Node result = config[key.toLatin1().data()];
    if (!result) {
      return QString();
    }
    return QString(result.as<std::string>().c_str());
  }
}
