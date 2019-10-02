#include "value.h"

#include <QDebug>

namespace Config {
  Value::Value() {
    value.setValue(nullptr);
    value_type = Config::Value::Type::Null;
  }

  Value::Value(int v) {
    value.setValue(v);
    value_type = Config::Value::Type::Integer;
  }

  Value::Value(const QString &v) {
    value.setValue(v);
    value_type = Config::Value::Type::String;
  }

  Value::Value(const QStringList &v) {
    value.setValue(v);
    value_type = Config::Value::Type::StringList;
  }

  Value::Value(const QList<int> &v) {
    value.setValue(v);
    value_type = Config::Value::Type::IntegerList;
  }

  Value::Value(const QMap<QString, Value> &v) {
    value.setValue(v);
    value_type = Config::Value::Type::Map;
  }

  Value::Type Value::type() const {
    return value_type;
  }

  bool Value::isNull() const {
    return type() == Config::Value::Type::Null;
  }

  QString Value::toString() const {
    switch (type()) {
      case Config::Value::Type::Null:
        return QString("<Value null>");
      case Config::Value::Type::Integer:
        return QString("<Value integer %1>").arg(get<int>());
      case Config::Value::Type::String:
        return QString("<Value string %1>").arg(get<QString>());
      case Config::Value::Type::StringList:
        return QString("<Value string list %1>").arg(get<QStringList>().join(", "));
      case Config::Value::Type::IntegerList: {
        QStringList sl;
        for (auto i : get<QList<int>>()) {
          sl << QString(i);
        }
        return QString("<Value integer list %1>").arg(sl.join(", "));
      }
      case Config::Value::Type::Map: {
        QStringList sl;
        for (auto i : get<QMap<QString, Value>>().toStdMap()) {
          QString key = i.first;
          Value val = i.second;
          sl << QString("%1:%2").arg(key, val.toString());
        }
        return QString("<Value map [%1]>").arg(sl.join(", "));
      }
    }
  }

  Config::Value::operator QString() const {
    return toString();
  }
}

