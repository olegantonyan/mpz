#ifndef VALUE_H
#define VALUE_H

#include <QVariant>
#include <QList>
#include <QString>
#include <QStringList>

namespace Config {
  class Value {
  public:
    enum Type {
      Null,
      Integer,
      String,
      StringList,
      IntegerList
    };

    Value();
    Value(int v);
    Value(const QString &v);
    Value(const QStringList &v);
    Value(const QList<int> &v);

    template<class T> T get() const {
      return value.value<T>();
    }

    enum Type type() const;
    bool isNull() const;

  private:
    QVariant value;
    enum Type value_type;
  };
}

#endif // VALUE_H
