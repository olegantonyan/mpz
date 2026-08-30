#include "fixture.h"

#include "playlist/playlist.h"
#include "playlist_ui/tageditordialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

// Writes real tags, so every case works on a private copy of a fixture.
class TestTagEditor : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void showsTheCurrentTagsForOneTrack();
  void savesOnlyTheEditedField();
  void savingWithNothingEditedWritesNothing();
  void mixedSelectionShowsThePlaceholder();
  void emptyValueErasesTheProperty();
  void navigationNeedsASinglePlaylistTrack();
  void navigationIsBoundedAndAutosaves();

private:
  QTemporaryDir dir;
  int counter = 0;

  QString copyFixture(const QString &name = "silence.flac");
  static QLineEdit *field(TagEditorDialog &dlg, const QString &name);
  // Dirty tracking hangs off textEdited, so fields have to be typed into.
  static void type(TagEditorDialog &dlg, const QString &name, const QString &text);
  static void accept(TagEditorDialog &dlg);
};

void TestTagEditor::initTestCase() {
  QVERIFY(dir.isValid());
}

void TestTagEditor::init() {
  counter++;
}

QString TestTagEditor::copyFixture(const QString &name) {
  const QString target = dir.filePath(QString("t%1_%2").arg(counter).arg(name));
  QFile::remove(target);
  if (!QFile::copy(QStringLiteral(AUDIO_FIXTURES_DIR) + "/" + name, target)) {
    return QString();
  }
  QFile::setPermissions(target, QFile::ReadOwner | QFile::WriteOwner);
  return target;
}

QLineEdit *TestTagEditor::field(TagEditorDialog &dlg, const QString &name) {
  return dlg.findChild<QLineEdit *>(name);
}

void TestTagEditor::type(TagEditorDialog &dlg, const QString &name, const QString &text) {
  auto *edit = field(dlg, name);
  edit->setFocus();
  edit->selectAll();
  QTest::keyClick(edit, Qt::Key_Delete);
  if (!text.isEmpty()) {
    QTest::keyClicks(edit, text);
  }
}

void TestTagEditor::accept(TagEditorDialog &dlg) {
  dlg.findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"))
      ->button(QDialogButtonBox::Save)
      ->click();
}

void TestTagEditor::showsTheCurrentTagsForOneTrack() {
  const QString path = copyFixture();
  QVERIFY(!path.isEmpty());
  {
    TagEditorDialog seed({Track(path)});
    type(seed, "lineEditArtist", "Artist One");
    type(seed, "lineEditAlbum", "Album One");
    accept(seed);
  }

  TagEditorDialog dlg({Track(path)});

  QCOMPARE(field(dlg, "lineEditArtist")->text(), QString("Artist One"));
  QCOMPARE(field(dlg, "lineEditAlbum")->text(), QString("Album One"));
  QCOMPARE(dlg.findChild<QLineEdit *>(QStringLiteral("lineEditFilename"))->text(),
           QFileInfo(path).fileName());
  QCOMPARE(dlg.findChild<QLabel *>(QStringLiteral("labelHeader"))->text(), QString("Editing 1 track(s)"));
}

void TestTagEditor::savesOnlyTheEditedField() {
  const QString path = copyFixture();
  {
    TagEditorDialog seed({Track(path)});
    type(seed, "lineEditArtist", "Keep Me");
    type(seed, "lineEditAlbum", "Keep Me Too");
    accept(seed);
  }

  TagEditorDialog dlg({Track(path)});
  QSignalSpy saved(&dlg, &TagEditorDialog::saved);
  type(dlg, "lineEditTitle", "New Title");
  accept(dlg);

  QCOMPARE(saved.count(), 1);
  const Track after(path);
  QCOMPARE(after.title(), QString("New Title"));
  QCOMPARE(after.artist(), QString("Keep Me"));
  QCOMPARE(after.album(), QString("Keep Me Too"));
}

void TestTagEditor::savingWithNothingEditedWritesNothing() {
  const QString path = copyFixture();
  {
    TagEditorDialog seed({Track(path)});
    type(seed, "lineEditArtist", "Untouched");
    accept(seed);
  }
  const QDateTime before = QFileInfo(path).lastModified();

  TagEditorDialog dlg({Track(path)});
  QSignalSpy saved(&dlg, &TagEditorDialog::saved);
  accept(dlg);

  QCOMPARE(saved.count(), 0);
  QCOMPARE(QFileInfo(path).lastModified(), before);
  QCOMPARE(Track(path).artist(), QString("Untouched"));
}

void TestTagEditor::mixedSelectionShowsThePlaceholder() {
  const QString one = copyFixture("silence.flac");
  const QString two = copyFixture("silence.mp3");
  for (const auto &pair : {qMakePair(one, QString("Artist A")), qMakePair(two, QString("Artist B"))}) {
    TagEditorDialog seed({Track(pair.first)});
    type(seed, "lineEditAlbum", "Same");
    type(seed, "lineEditArtist", pair.second);
    accept(seed);
  }

  TagEditorDialog dlg({Track(one), Track(two)});

  QCOMPARE(field(dlg, "lineEditAlbum")->text(), QString("Same"));
  // Differing values leave the field empty and show the placeholder, so typing
  // into it is an explicit choice rather than an accidental overwrite.
  QVERIFY(field(dlg, "lineEditArtist")->text().isEmpty());
  QCOMPARE(field(dlg, "lineEditArtist")->placeholderText(), QString("<multiple values>"));
  QVERIFY(dlg.findChild<QLineEdit *>(QStringLiteral("lineEditFilename"))->isHidden());
  QCOMPARE(dlg.findChild<QLabel *>(QStringLiteral("labelHeader"))->text(), QString("Editing 2 track(s)"));
}

void TestTagEditor::emptyValueErasesTheProperty() {
  const QString path = copyFixture();
  {
    TagEditorDialog seed({Track(path)});
    type(seed, "lineEditAlbumArtist", "Various");
    type(seed, "lineEditDiscNumber", "2");
    accept(seed);
  }
  {
    Track t(path);
    QCOMPARE(t.album_artist(), QString("Various"));
    QCOMPARE(t.disc_number(), QString("2"));
  }

  TagEditorDialog dlg({Track(path)});
  type(dlg, "lineEditAlbumArtist", "");
  type(dlg, "lineEditDiscNumber", "");
  accept(dlg);

  const Track after(path);
  QVERIFY(after.album_artist().isEmpty());
  QVERIFY(after.disc_number().isEmpty());
}

void TestTagEditor::navigationNeedsASinglePlaylistTrack() {
  const QString path = copyFixture();
  TagEditorDialog without_playlist({Track(path)});
  QVERIFY(without_playlist.findChild<QWidget *>(QStringLiteral("navContainer"))->isHidden());

  auto playlist = std::make_shared<Playlist::Playlist>();
  playlist->load({Track(path), Track(copyFixture("silence.mp3"))});
  TagEditorDialog multi({playlist->tracks().at(0), playlist->tracks().at(1)}, playlist);
  QVERIFY(multi.findChild<QWidget *>(QStringLiteral("navContainer"))->isHidden());
}

void TestTagEditor::navigationIsBoundedAndAutosaves() {
  auto playlist = std::make_shared<Playlist::Playlist>();
  const QString first = copyFixture("silence.flac");
  const QString second = copyFixture("silence.mp3");
  playlist->load({Track(first), Track(second)});

  TagEditorDialog dlg({playlist->tracks().first()}, playlist);
  auto *prev = dlg.findChild<QPushButton *>(QStringLiteral("previousButton"));
  auto *next = dlg.findChild<QPushButton *>(QStringLiteral("nextButton"));
  auto *autosave = dlg.findChild<QCheckBox *>(QStringLiteral("autosaveCheckBox"));

  QCOMPARE(dlg.findChild<QLabel *>(QStringLiteral("labelHeader"))->text(), QString("Track 1 of 2"));
  QVERIFY(!prev->isEnabled());
  QVERIFY(next->isEnabled());

  autosave->setChecked(true);
  type(dlg, "lineEditArtist", "Autosaved");
  next->click();

  QCOMPARE(dlg.findChild<QLabel *>(QStringLiteral("labelHeader"))->text(), QString("Track 2 of 2"));
  QVERIFY(prev->isEnabled());
  QVERIFY(!next->isEnabled());
  QCOMPARE(Track(first).artist(), QString("Autosaved"));
}

MPZ_GUI_TEST_MAIN(TestTagEditor)
#include "tst_tageditor.moc"
