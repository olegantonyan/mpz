#include "value.h"

#include <QDebug>

namespace Config {
  Value::Value() {
    //value.setValue(nullptr);
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

  Value::Value(const QMap<QString, Value> &v) {
    value.setValue(v);
    value_type = Config::Value::Type::Map;
  }

  Value::Value(const QList<Value> &v) {
    value.setValue(v);
    value_type = Config::Value::Type::List;
    if (v.size() > 0) { // assuming all values in the list are the same type
      setListType(v.first().type());
    }
  }

  Value::Value(bool v) {
    value.setValue(v);
    value_type = Config::Value::Type::Boolean;
  }

  Value::Type Value::type() const {
    return value_type;
  }

  Value::Type Value::listType() const {
    return list_elements_type;
  }

  bool Value::isNull() const {
    return type() == Config::Value::Type::Null;
  }

  void Value::setListType(Value::Type t) {
    list_elements_type = t;
  }

  QString Value::toString() const {
    switch (type()) {
      case Config::Value::Type::Null:
        return QString("<Value null>");
      case Config::Value::Type::Boolean:
        return QString("<Value boolean %1>").arg(get<bool>());
      case Config::Value::Type::Integer:
        return QString("<Value integer %1>").arg(get<int>());
      case Config::Value::Type::String:
        return QString("<Value string %1>").arg(get<QString>());
      case Config::Value::Type::Map: {
        QStringList sl;
        for (auto i : get<QMap<QString, Value>>().toStdMap()) {
          QString key = i.first;
          Value val = i.second;
          sl << QString("%1:%2").arg(key, val.toString());
        }
        return QString("<Value map {%1}>").arg(sl.join(", "));
      }
      case Config::Value::Type::List: {
        QStringList sl;
        for (auto i : get<QList<Value>>()) {
          sl << i.toString();
        }

        return QString("<Value map list [%1]>").arg(sl.join(", "));
      }
    }
    return QString();
  }

  Config::Value::operator QString() const {
    return toString();
  }
}
