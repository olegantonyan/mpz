#ifndef CONFIG_VALUE_H
#define CONFIG_VALUE_H

#include <QVariant>
#include <QList>
#include <QString>
#include <QStringList>
#include <QMap>

namespace Config {
  class Value {
  public:
    enum Type {
      Null,
      Integer,
      String,
      Map,
      List,
      Boolean
    };

    Value();
    Value(int v);
    Value(const QString &v);
    Value(const QMap<QString, Value> &v);
    Value(const QList<Value> &v);
    Value(bool v);

    template<class T> T get() const {
      return value.value<T>();
    }

    Type type() const;
    Type listType() const;
    bool isNull() const;

    void setListType(Type t);

    operator QString() const;
    QString toString() const;

  private:
    QVariant value;
    Type value_type;
    Type list_elements_type;
  };
}

Q_DECLARE_METATYPE(Config::Value::Type)
Q_DECLARE_METATYPE(Config::Value)

#endif // CONFIG_VALUE_H
