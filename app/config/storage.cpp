#include "storage.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QFile>

namespace Config {
  Storage::Storage(const QString &filepath) {
    if (QFile::exists(filepath)) {

      YAML::Node config = YAML::LoadFile(filepath.toStdString());
      if (config["hello"]) {
        qDebug() << "hello " << QString(config["hello"].as<std::string>().c_str()) << "\n";
      } else {
        qDebug() << "nope";
      }

    } else {
      qDebug() << "file does not exists" << filepath;
    }



  }
}
