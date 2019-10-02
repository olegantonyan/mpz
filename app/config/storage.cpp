#include "storage.h"

#include <QDebug>
#include <QFile>

namespace Config {
  Storage::Storage(const QString &path) : filepath(path), changed(false) {
    reload();
    //qDebug() << data;
  }

  Storage::~Storage() {
    save();
  }

  Value Storage::get(const QString &key, bool *ok) const {
    if (!data.contains(key)) {
      if (ok) {
        *ok = false;
      }
      return Config::Value();
    }
    if (ok) {
      *ok = true;
    }
    return data[key];
  }

  bool Storage::set(const QString &key, const Config::Value &value) {
    data[key] = value;
    changed = true;
    return true;
  }

  QByteArray Storage::getByteArray(const QString &key, bool *ok) const {
    QByteArray result;
    for (auto i : getIntList(key, ok)) {
      result.append(static_cast<char>(i));
    }
    return result;
  }

  bool Storage::set(const QString &key, const QByteArray &value) {
    QList<int> intlist;
    for (auto i : value) {
      intlist << i;
    }
    return set(key, intlist);
  }

  QList<int> Storage::getIntList(const QString &key, bool *ok) const {
    Config::Value value = get(key, ok);
    if (value.isNull() || value.type() != Config::Value::Type::IntegerList || (ok && !*ok)) {
      return QList<int>();
    }
    return value.get<QList<int>>();
  }

  bool Storage::set(const QString &key, const QList<int> &value) {
    return set(key, Config::Value(value));
  }

  QStringList Storage::getStringList(const QString &key, bool *ok) const {
    return get(key, ok).get<QStringList>();
  }

  bool Storage::set(const QString &key, const QStringList &value) {
    return set(key, Config::Value(value));
  }

  bool Storage::save() {
    if (!changed) {
      return true;
    }
    QFile file(filepath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
      qWarning() << "error opening file for writing" << filepath << ":" << file.errorString();
      return false;
    }

    YAML::Emitter emitter;
    emitter << serialize(data);
    bool ok = file.write(emitter.c_str()) == static_cast<qint64>(emitter.size());
    changed = !ok;
    return ok;
  }

  bool Storage::reload() {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << "error opening file for reading" << filepath << ":" << file.errorString();
      return false;
    }
    YAML::Node config = YAML::Load(file.readAll().toStdString());

    for (auto i : parse(config).toStdMap()) {
      data.insert(i.first, i.second);
    }

    return true;
  }

  Config::Value Storage::castScalar(const QString &str) const {
    bool ok = false;
    auto integer = str.toInt(&ok);
    if (ok) {
      return Config::Value(integer);
    } else {
      return Config::Value(str);
    }
  }

  Config::Value Storage::castSequence(const QStringList &strl) const {
    bool all_ok = !strl.isEmpty();
    QList<int> intlist;

    for (auto i : strl) {
      Config::Value v = castScalar(i);
      if (v.type() == Config::Value::Type::Integer) {
        intlist.append(v.get<int>());
      } else {
        all_ok = false;
        break;
      }
    }
    if (all_ok) {
      return Config::Value(QList<int>(intlist));
    }
    return Config::Value(strl);
  }

  QMap<QString, Value> Storage::parse(const YAML::Node &begin_node) const {
    QMap<QString, Value> result;

    for (YAML::const_iterator it = begin_node.begin(); it != begin_node.end(); ++it) {
      auto key = QString::fromStdString(it->first.as<std::string>());

      auto node = it->second;
      Config::Value value;

      if (node.IsScalar()) {
        auto string = QString::fromStdString(node.as<std::string>());
        value = castScalar(string);
      } else if(node.IsSequence()) {
        QStringList list;
        for (size_t i = 0; i < node.size(); i++) {
          auto val = node[i];
          list << QString::fromStdString(val.as<std::string>());
        }
        value = castSequence(list);
      } else if(node.IsNull()) {
        // it's null by default, nothing to do
      } else if(node.IsMap()) {
        value = parse(node);
      } else {
        qWarning() << "unknown yaml type";
      }
      result.insert(key, value);
    }
    return result;
  }

  YAML::Node Storage::serialize(const QMap<QString, Config::Value> &dt) const {

    YAML::Node result;

    for(auto i : dt.toStdMap()) {
      std::string key = i.first.toStdString();
      Config::Value value = i.second;

      switch (value.type()) {
        case Config::Value::Type::Integer:
          result[key] = value.get<int>();
          break;
        case Config::Value::Type::String:
          result[key] = value.get<QString>().toStdString();
          break;
        case Config::Value::Type::StringList:
          result[key] = std::vector<std::string>();
          for (auto i : value.get<QStringList>()) {
            result[key].push_back(i.toStdString());
          }
          break;
        case Config::Value::Type::IntegerList:
          result[key] = std::vector<int>();
          for (auto i : value.get<QList<int>>()) {
            result[key].push_back(i);
          }
          break;
        case Config::Value::Type::Map:
          result[key] = serialize(value.get<QMap<QString, Config::Value>>());
          break;
        default:
          break;
      }
    }
    return result;
  }
}
