#ifndef VALUE_H
#define VALUE_H

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
      List
    };

    Value();
    Value(int v);
    Value(const QString &v);
    Value(const QMap<QString, Value> &v);
    Value(const QList<Value> &v);

    template<class T> T get() const {
      return value.value<T>();
    }

    enum Type type() const;
    enum Type listType() const;
    bool isNull() const;

    void setListType(enum Type t);

    operator QString() const;
    QString toString() const;

  private:
    QVariant value;
    enum Type value_type;
    enum Type list_elements_type;
  };
}

Q_DECLARE_METATYPE(Config::Value)

#endif // VALUE_H
