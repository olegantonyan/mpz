#include "storage.h"

#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

namespace Config {
  Storage::Storage(const QString &filename) : changed(false) {
    auto default_filedir = configPath();
    auto default_filepath = QString("%1/%2").arg(default_filedir).arg(filename);
    qDebug() << "config file: " << default_filepath;

    if (QFile::exists(filename)) {
      filepath = filename;
    } else {
      filepath = default_filepath;
      if (!QFile::exists(default_filepath)) {
        QDir().mkpath(default_filedir);
      }
    }

    reload();
  }

  Storage::~Storage() {
    save();
  }

  QString Storage::configPath() {
    auto cfg_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return QString("%1/mpz").arg(cfg_path);
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
    if (value.isNull() || value.type() != Config::Value::Type::List || value.listType() != Config::Value::Type::Integer || (ok && !*ok)) {
      return QList<int>();
    }
    auto i = value.get<QList<Config::Value>>();
    QList<int> result;
    for (auto j : i) {
      result << j.get<int>();
    }
    return result;
  }

  bool Storage::set(const QString &key, const QList<int> &value) {
    QList<Config::Value> vl;
    for (auto i : value) {
      vl.append(Config::Value(i));
    }
    auto i = Config::Value(vl);
    i.setListType(Config::Value::Type::Integer);
    return set(key, i);
  }

  bool Storage::save() {
    if (!changed) {
      return true;
    }
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
      qWarning() << "error opening file for writing" << filepath << ":" << file.errorString();
      return false;
    }

    YAML::Emitter emitter;
    emitter << serialize(data);
    //qDebug() << emitter.c_str();
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
    // try boolean first
    if (str.compare("true", Qt::CaseInsensitive) == 0 ||
        str.compare("yes", Qt::CaseInsensitive) == 0 ||
        str.compare("y", Qt::CaseInsensitive) == 0 ||
        str.compare("on", Qt::CaseInsensitive) == 0) {
      return Config::Value(true);
    } else if (str.compare("false", Qt::CaseInsensitive) == 0 ||
               str.compare("no", Qt::CaseInsensitive) == 0 ||
               str.compare("n", Qt::CaseInsensitive) == 0 ||
               str.compare("off", Qt::CaseInsensitive) == 0) {
      return Config::Value(false);
    }

    // then integer
    bool ok = false;
    auto integer = str.toInt(&ok);
    if (ok) {
      return Config::Value(integer);
    } else {
      return Config::Value(str);
    }
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
        QList<Config::Value> values;
        enum Config::Value::Type list_type = Config::Value::Type::Null;
        for (size_t i = 0; i < node.size(); i++) {
          auto val = node[i];
          if (val.IsScalar()) {
            auto sc = QString::fromStdString(val.as<std::string>());
            auto v = Config::Value(castScalar(sc));
            values.append(v);
            list_type = v.type();
          } else if (val.IsMap()) {
            values.append(parse(val));
            list_type = Config::Value::Type::Map;
          }
        }
        value = values;
        value.setListType(list_type);
      } else if(node.IsNull()) {
        // it's null by default, nothing to do
      } else if(node.IsMap()) {
        value = parse(node);
      } else {
        qWarning() << "unknown yaml type at key " << key;
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
        case Config::Value::Type::Boolean:
          result[key] = value.get<bool>();
          break;
        case Config::Value::Type::String:
          result[key] = value.get<QString>().toStdString();
          break;
        case Config::Value::Type::List:
          if (value.listType() == Config::Value::Type::Integer) {
            result[key] = std::vector<int>();
            for (auto i : value.get<QList<Config::Value>>()) {
              result[key].push_back(i.get<int>());
            }
          } else if (value.listType() == Config::Value::Type::String) {
            result[key] = std::vector<std::string>();
            for (auto i : value.get<QList<Config::Value>>()) {
              result[key].push_back(i.get<QString>().toStdString());
            }
          } else if (value.listType() == Config::Value::Type::Map) {
            result[key] = std::vector< std::map<std::string, YAML::Node> >();
            for (auto i : value.get<QList<Config::Value>>()) {
              result[key].push_back(serialize(i.get<QMap<QString, Config::Value>>()));
            }
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
