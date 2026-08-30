#include <QtTest>
#include <QSet>
#include <QMap>
#include <QFile>
#include <QWidget>
#include <QShortcut>
#include <QTemporaryDir>

#include "shortcuts.h"

class TestShortcuts : public QObject {
  Q_OBJECT
private slots:
  void defaults_haveUniqueConfigKeys();
  void defaults_haveNoDuplicateSequences();
  void resolve_appliesAnOverride();
  void resolve_ignoresUnknownKeys();
  void resolve_keepsDefaultOnUnparseableValue_data();
  void resolve_keepsDefaultOnUnparseableValue();
  void resolve_emptyStringUnbinds();
  void resolve_acceptsBareKeys_data();
  void resolve_acceptsBareKeys();
  void resolve_overrideBeatsCollidingDefault();
  void resolve_earlierOverrideWinsAgainstLaterOverride();
  void resolve_neverLeavesDuplicateSequences();
  void applyOverrides_persistsAndKeepsUnknownKeys();
  void applyOverrides_emptyDropsOwnKeysOnly();
  void applyOverrides_rebindsTheLiveQShortcut();

private:
  static Shortcuts::Spec specFor(const QVector<Shortcuts::Spec> &table, const QString &key);
  static QString firstLocallyRegisteredKey();
};

Shortcuts::Spec TestShortcuts::specFor(const QVector<Shortcuts::Spec> &table, const QString &key) {
  for (const auto &spec : table) {
    if (spec.key == key) {
      return spec;
    }
  }
  return Shortcuts::Spec{};
}

void TestShortcuts::defaults_haveUniqueConfigKeys() {
  QSet<QString> seen;
  for (const auto &spec : Shortcuts::defaults()) {
    QVERIFY2(!spec.key.isEmpty(), qPrintable(spec.description));
    QVERIFY2(!seen.contains(spec.key), qPrintable(spec.key));
    seen.insert(spec.key);
  }
  QCOMPARE(seen.size(), Shortcuts::defaults().size());
}

void TestShortcuts::defaults_haveNoDuplicateSequences() {
  const auto table = Shortcuts::defaults();
  for (int i = 0; i < table.size(); i++) {
    if (table[i].sequence.isEmpty()) {
      continue;
    }
    for (int j = i + 1; j < table.size(); j++) {
      QVERIFY2(table[i].sequence != table[j].sequence,
               qPrintable(table[i].key + " and " + table[j].key + " share " +
                          table[i].sequence.toString(QKeySequence::PortableText)));
    }
  }
}

void TestShortcuts::resolve_appliesAnOverride() {
  const auto table = Shortcuts::resolve({{"next", "Ctrl+Alt+N"}});
  QCOMPARE(specFor(table, "next").sequence,
           QKeySequence::fromString("Ctrl+Alt+N", QKeySequence::PortableText));
}

void TestShortcuts::resolve_ignoresUnknownKeys() {
  const auto table = Shortcuts::resolve({{"not_an_action", "Ctrl+Alt+N"}});
  QCOMPARE(table.size(), Shortcuts::defaults().size());
  for (int i = 0; i < table.size(); i++) {
    QCOMPARE(table[i].sequence, Shortcuts::defaults()[i].sequence);
  }
}

void TestShortcuts::resolve_keepsDefaultOnUnparseableValue_data() {
  QTest::addColumn<QString>("value");
  QTest::newRow("prose") << "this is not a key sequence";
  QTest::newRow("short prose") << "this is garbage";
  QTest::newRow("punctuation") << "!!!";
  QTest::newRow("unknown key") << "Ctrl+Nonsense";
  QTest::newRow("modifier only") << "Ctrl+";
  QTest::newRow("number") << "12345";
}

void TestShortcuts::resolve_keepsDefaultOnUnparseableValue() {
  QFETCH(QString, value);
  const auto table = Shortcuts::resolve({{"next", value}});
  QCOMPARE(specFor(table, "next").sequence, specFor(Shortcuts::defaults(), "next").sequence);
}

void TestShortcuts::resolve_emptyStringUnbinds() {
  const auto table = Shortcuts::resolve({{"next", ""}});
  QVERIFY(specFor(table, "next").sequence.isEmpty());
}

void TestShortcuts::resolve_acceptsBareKeys_data() {
  QTest::addColumn<QString>("value");
  QTest::newRow("Y") << "Y";
  QTest::newRow("N") << "N";
  QTest::newRow("5") << "5";
  QTest::newRow("Space") << "Space";
}

void TestShortcuts::resolve_acceptsBareKeys() {
  QFETCH(QString, value);
  const auto table = Shortcuts::resolve({{"next", value}});
  QCOMPARE(specFor(table, "next").sequence.toString(QKeySequence::PortableText), value);
}

void TestShortcuts::resolve_overrideBeatsCollidingDefault() {
  const auto stolen = specFor(Shortcuts::defaults(), "focus_library").sequence;
  QVERIFY(!stolen.isEmpty());

  const auto table = Shortcuts::resolve({{"next", stolen.toString(QKeySequence::PortableText)}});
  QCOMPARE(specFor(table, "next").sequence, stolen);
  QVERIFY(specFor(table, "focus_library").sequence.isEmpty());
}

void TestShortcuts::resolve_earlierOverrideWinsAgainstLaterOverride() {
  const auto &defs = Shortcuts::defaults();
  QString first;
  QString second;
  for (const auto &spec : defs) {
    if (spec.description.isEmpty()) {
      continue;
    }
    if (first.isEmpty()) {
      first = spec.key;
    } else if (second.isEmpty()) {
      second = spec.key;
    }
  }
  QVERIFY(!second.isEmpty());

  const auto table = Shortcuts::resolve({{first, "Ctrl+Alt+Shift+F9"}, {second, "Ctrl+Alt+Shift+F9"}});
  QVERIFY(!specFor(table, first).sequence.isEmpty());
  QVERIFY(specFor(table, second).sequence.isEmpty());
}

void TestShortcuts::resolve_neverLeavesDuplicateSequences() {
  QMap<QString, QString> overrides;
  for (const auto &spec : Shortcuts::defaults()) {
    overrides.insert(spec.key, "Ctrl+Alt+Shift+F9");
  }
  const auto table = Shortcuts::resolve(overrides);

  int bound = 0;
  for (const auto &spec : table) {
    if (!spec.sequence.isEmpty()) {
      bound++;
    }
  }
  QCOMPARE(bound, 1);
}

QString TestShortcuts::firstLocallyRegisteredKey() {
  for (const auto &spec : Shortcuts::defaults()) {
    if (spec.registerLocal && !spec.description.isEmpty()) {
      return spec.key;
    }
  }
  return QString();
}

void TestShortcuts::applyOverrides_persistsAndKeepsUnknownKeys() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
  {
    QFile f(dir.filePath(QStringLiteral("global.yml")));
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write("shortcuts:\n  not_an_action: Ctrl+K\n");
  }

  QWidget parent;
  Config::Global global_conf;
  Config::Local local_conf;
  local_conf.saveDisableQhotkey(true);
  Shortcuts sc(global_conf, local_conf, &parent);

  sc.applyOverrides({{"next", "Ctrl+Alt+N"}});

  QCOMPARE(sc.sequenceFor(Shortcuts::Action::Next),
           QKeySequence::fromString("Ctrl+Alt+N", QKeySequence::PortableText));
  const auto stored = global_conf.shortcuts();
  QCOMPARE(stored.value("next"), QString("Ctrl+Alt+N"));
  QCOMPARE(stored.value("not_an_action"), QString("Ctrl+K"));

  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestShortcuts::applyOverrides_emptyDropsOwnKeysOnly() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
  {
    QFile f(dir.filePath(QStringLiteral("global.yml")));
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write("shortcuts:\n  next: Ctrl+Alt+N\n  not_an_action: Ctrl+K\n");
  }

  QWidget parent;
  Config::Global global_conf;
  Config::Local local_conf;
  local_conf.saveDisableQhotkey(true);
  Shortcuts sc(global_conf, local_conf, &parent);

  sc.applyOverrides({});

  QCOMPARE(sc.sequenceFor(Shortcuts::Action::Next), specFor(Shortcuts::defaults(), "next").sequence);
  const auto stored = global_conf.shortcuts();
  QVERIFY(!stored.contains("next"));
  QCOMPARE(stored.value("not_an_action"), QString("Ctrl+K"));

  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestShortcuts::applyOverrides_rebindsTheLiveQShortcut() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());

  const QString key = firstLocallyRegisteredKey();
  QVERIFY(!key.isEmpty());
  const auto rebound = QKeySequence::fromString("Ctrl+Alt+Shift+F9", QKeySequence::PortableText);

  QWidget parent;
  Config::Global global_conf;
  Config::Local local_conf;
  local_conf.saveDisableQhotkey(true);
  Shortcuts sc(global_conf, local_conf, &parent);

  const auto before = parent.findChildren<QShortcut *>().size();
  QVERIFY(before > 0);

  sc.applyOverrides({{key, rebound.toString(QKeySequence::PortableText)}});

  int matching = 0;
  for (const auto *shortcut : parent.findChildren<QShortcut *>()) {
    if (shortcut->key() == rebound) {
      matching++;
    }
  }
  QCOMPARE(matching, 1);
  QCOMPARE(parent.findChildren<QShortcut *>().size(), before);

  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

// not GUILESS: QKeySequence::Preferences needs a platform theme
QTEST_MAIN(TestShortcuts)
#include "tst_shortcuts.moc"
