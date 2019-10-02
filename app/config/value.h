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
      StringList,
      IntegerList,
      Map
    };

    Value();
    Value(int v);
    Value(const QString &v);
    Value(const QStringList &v);
    Value(const QList<int> &v);
    Value(const QMap<QString, Value> &v);

    template<class T> T get() const {
      return value.value<T>();
    }

    enum Type type() const;
    bool isNull() const;

    operator QString() const;
    QString toString() const;

  private:
    QVariant value;
    enum Type value_type;
  };
}

Q_DECLARE_METATYPE(Config::Value)

#endif // VALUE_H
