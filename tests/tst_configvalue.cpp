#include <QtTest>

#include "config/value.h"

class TestConfigValue : public QObject {
  Q_OBJECT
private slots:
  void defaultIsNull();
  void integerConstructorAndType();
  void stringConstructorAndType();
  void booleanConstructorAndType();
  void mapConstructorAndType();
  void listConstructorAndElementType();
  void listElementTypeInferredFromFirst();
  void setListTypeOverridesInference();
  void emptyListHasNullElementType();
  void toStringContainsTypeMarker();
  void operatorQStringMirrorsToString();
};

void TestConfigValue::defaultIsNull() {
  Config::Value v;
  QVERIFY(v.isNull());
  QCOMPARE(v.type(), Config::Value::Null);
}

void TestConfigValue::integerConstructorAndType() {
  Config::Value v(42);
  QCOMPARE(v.type(), Config::Value::Integer);
  QCOMPARE(v.get<int>(), 42);
  QVERIFY(!v.isNull());
}

void TestConfigValue::stringConstructorAndType() {
  Config::Value v(QStringLiteral("hello"));
  QCOMPARE(v.type(), Config::Value::String);
  QCOMPARE(v.get<QString>(), QStringLiteral("hello"));
}

void TestConfigValue::booleanConstructorAndType() {
  Config::Value t(true);
  Config::Value f(false);
  QCOMPARE(t.type(), Config::Value::Boolean);
  QCOMPARE(f.type(), Config::Value::Boolean);
  QCOMPARE(t.get<bool>(), true);
  QCOMPARE(f.get<bool>(), false);
}

void TestConfigValue::mapConstructorAndType() {
  QMap<QString, Config::Value> m;
  m.insert(QStringLiteral("a"), Config::Value(1));
  m.insert(QStringLiteral("b"), Config::Value(QStringLiteral("x")));
  Config::Value v(m);
  QCOMPARE(v.type(), Config::Value::Map);
  auto out = v.get<QMap<QString, Config::Value>>();
  QCOMPARE(out.size(), 2);
  QCOMPARE(out.value(QStringLiteral("a")).get<int>(), 1);
  QCOMPARE(out.value(QStringLiteral("b")).get<QString>(), QStringLiteral("x"));
}

void TestConfigValue::listConstructorAndElementType() {
  QList<Config::Value> list = { Config::Value(1), Config::Value(2), Config::Value(3) };
  Config::Value v(list);
  QCOMPARE(v.type(), Config::Value::List);
  QCOMPARE(v.listType(), Config::Value::Integer);
  QCOMPARE(v.get<QList<Config::Value>>().size(), 3);
}

void TestConfigValue::listElementTypeInferredFromFirst() {
  QList<Config::Value> list = { Config::Value(QStringLiteral("a")), Config::Value(QStringLiteral("b")) };
  Config::Value v(list);
  QCOMPARE(v.listType(), Config::Value::String);
}

void TestConfigValue::setListTypeOverridesInference() {
  QList<Config::Value> list = { Config::Value(1) };
  Config::Value v(list);
  QCOMPARE(v.listType(), Config::Value::Integer);
  v.setListType(Config::Value::String);
  QCOMPARE(v.listType(), Config::Value::String);
}

void TestConfigValue::emptyListHasNullElementType() {
  Config::Value v(QList<Config::Value>{});
  QCOMPARE(v.type(), Config::Value::List);
  QCOMPARE(v.listType(), Config::Value::Null);
}

void TestConfigValue::toStringContainsTypeMarker() {
  QCOMPARE(Config::Value().toString(), QStringLiteral("<Value null>"));
  QVERIFY(Config::Value(7).toString().contains(QStringLiteral("integer")));
  QVERIFY(Config::Value(QStringLiteral("hi")).toString().contains(QStringLiteral("string")));
  QVERIFY(Config::Value(true).toString().contains(QStringLiteral("boolean")));
}

void TestConfigValue::operatorQStringMirrorsToString() {
  Config::Value v(QStringLiteral("hi"));
  QString implicit = v;
  QCOMPARE(implicit, v.toString());
}

QTEST_GUILESS_MAIN(TestConfigValue)
#include "tst_configvalue.moc"
