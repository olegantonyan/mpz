#include "storage.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QFile>
#include <iostream>

namespace Config {
  Storage::Storage(const QString &path) : filepath(path) {
    QFile file(filepath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      YAML::Node config = YAML::Load(file.readAll().toStdString());

      for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
        const QString key = QString::fromStdString(it->first.as<std::string>());

        YAML::Node node = it->second;
        QVariant value;

        if (node.IsScalar()) {
          value = QVariant(QString::fromStdString(node.as<std::string>()));
        } else if(node.IsSequence()) {

        } else if(node.IsNull()) {

        } else if(node.IsMap()) {

        } else {

        }
        data.insert(key, value);


       // std::cout << it->first.as<std::string>() << " is " << it->second.as<std::string>() << "\n";
      }
    } else {
      qWarning() << "error opening file" << filepath << ":" << file.errorString();
    }

    qDebug() << data;
  }

  QVariant Storage::get(const QString &key, bool *ok) {
    if (!data.contains(key)) {
      if (ok) {
        *ok = false;
      }
      return QVariant();
    }
    *ok = true;
    return data[key];
  }

  bool Storage::set(const QString &key, const QVariant &value) {
    data[key] = value;



    return true;
  }

  bool Storage::save() {
    QFile file(filepath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      return false;
    }

    /*if (!QFile::exists(filepath)) {
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
*/
    return true;
  }
}
