#include "storage.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QFile>
#include <iostream>

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

  bool Storage::setString(const QString &key, const QString &value) {
    if (!QFile::exists(filepath)) {
      return false;
    }
    YAML::Node config, root = YAML::LoadFile(filepath.toStdString());
    root[key.toLatin1().data()] = value.toLatin1().data();
    QFile f(filepath);
    if (!f.open(QIODevice::ReadWrite | QIODevice::Text)) {
      return false;
    }
    YAML::Emitter emitter;
    emitter << root;
    f.write(emitter.c_str());

    return true;
  }

  bool Storage::setInt(const QString &key, int value) {
    if (!QFile::exists(filepath)) {
      return false;
    }
    YAML::Node config, root = YAML::LoadFile(filepath.toStdString());
    root[key.toLatin1().data()] = value;
    QFile f(filepath);
    if (!f.open(QIODevice::ReadWrite | QIODevice::Text)) {
      return false;
    }
    YAML::Emitter emitter;
    emitter << root;
    f.write(emitter.c_str());

    return true;
  }

  QStringList Storage::getStringList(const QString &key, bool *ok) {

  }

  bool Storage::setStringList(const QString &key, const QStringList &value) {

  }
}
