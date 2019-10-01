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

  Value::Type Value::type() const {
    return value_type;
  }

  bool Value::isNull() const {
    return type() == Config::Value::Type::Null;
  }
}

