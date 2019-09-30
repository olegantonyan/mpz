#include "storage.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QFile>
#include <iostream>

namespace Config {
  Storage::Storage(const QString &path) : filepath(path) {
    reload();
    //qDebug() << "flle" << filepath << "data" << data;
  }

  QVariant Storage::get(const QString &key, bool *ok) const {
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
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
      qWarning() << "error opening file for writing" << filepath << ":" << file.errorString();
      return false;
    }

    YAML::Node root;
    for(auto i : data.toStdMap()) {
      std::string key = i.first.toStdString();
      QVariant value = i.second;
      switch (value.type()) {
        case QVariant::Int:
          root[key] = value.value<int>();
          break;
        case QVariant::String:
          root[key] = value.value<QString>().toStdString();
          break;
        /*case QVariant::List:
          qWarning() << "TODO: List not supported yet";
          break;*/
        default:
          auto tname = QString(value.typeName());
          if (tname == "QList<int>") {
            root[key] = std::vector<int>();
            auto list = value.value<QList<int>>();
            for (auto i : list) {
              root[key].push_back(i);
            }
          } else if (tname == "QStringList") {
            auto list = value.value<QStringList>();
            for (auto i : list) {
              root[key].push_back(i.toStdString());
            }
          } else {
            qWarning() << "unsupported QVariant type" << value.type() << "|" << tname << "|" << value.userType();
            return false;
          }
      }
    }

    YAML::Emitter emitter;
    emitter << root;
    //qDebug() << emitter.c_str();
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
        value = castScalar(string);
      } else if(node.IsSequence()) {
        QStringList list;
        for (size_t i = 0; i < node.size(); i++) {
          auto val = node[i];
          list << QString::fromStdString(val.as<std::string>());
        }
        //qDebug() << "list" << list;
        value = castSequence(list);
        //qDebug() << "value" << value;

      } else if(node.IsNull()) {

      } else if(node.IsMap()) {

      } else {

      }
      data.insert(key, value);
    }
    return true;
  }

  QVariant Storage::castScalar(const QString &str) const {
    bool ok = false;
    auto integer = str.toInt(&ok);
    if (ok) {
      return QVariant(integer);
    } else {
      return QVariant(str);
    }
  }

  QVariant Storage::castSequence(const QStringList &strl) const {
    bool all_ok = !strl.isEmpty();
    QList<int> intlist;

    for (auto i : strl) {
      bool ok = false;
      auto integer = i.toInt(&ok);
      if (ok) {
        intlist.append(integer);
      } else {
        all_ok = false;
        break;
      }
    }
    if (all_ok) {
      return QVariant::fromValue<QList<int>>(intlist);
    }
    return QVariant::fromValue(strl);
  }
}
