#include "storage.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QFile>
#include <iostream>

namespace Config {
  Storage::Storage(const QString &path) : filepath(path) {
    reload();
    qDebug() << "flle" << filepath << "data" << data;
  }

  QVariant Storage::get(const QString &key, bool *ok) {
    if (!data.contains(key)) {
      if (ok) {
        *ok = false;
      }
      return QVariant();
    }
    if (ok) {
      *ok = true;
    }
    return data[key];
  }

  bool Storage::set(const QString &key, const QVariant &value) {
    if (!value.isValid()) {
      return false;
    }
    data[key] = value;
    return true;
  }

  bool Storage::save() {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      qWarning() << "error opening file for writing" << filepath << ":" << file.errorString();
      return false;
    }

    YAML::Node root;
    for(auto i : data.toStdMap()) {
      std::string key = i.first.toStdString();
      QVariant value = i.second;
      switch (value.userType()) {
        case QVariant::Int:
          root[key] = value.value<int>();
          break;
        case QVariant::String:
          root[key] = value.value<QString>().toStdString();
          break;
        case QVariant::List:
          qWarning() << "TODO: QList not supported yet";
          break;
        default:
          qWarning() << "unsupported QVariant meta type" << value.userType();
          return false;
      }
    }

    YAML::Emitter emitter;
    emitter << root;
    qDebug() << emitter.c_str();
    file.write(emitter.c_str());

    return true;
  }

  bool Storage::reload() {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << "error opening file for reading" << filepath << ":" << file.errorString();
      return false;
    }
    YAML::Node config = YAML::Load(file.readAll().toStdString());

    for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
      auto key = QString::fromStdString(it->first.as<std::string>());

      auto node = it->second;
      QVariant value;

      if (node.IsScalar()) {
        auto string = QString::fromStdString(node.as<std::string>());
        bool ok = false;
        auto integer = string.toInt(&ok);
        if (ok) {
          value = QVariant(integer);
        } else {
          value = QVariant(string);
        }
      } else if(node.IsSequence()) {

      } else if(node.IsNull()) {

      } else if(node.IsMap()) {

      } else {

      }
      data.insert(key, value);
    }
    return true;
  }
}
