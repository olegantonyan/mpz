#include "tageditordialog.h"
#include "ui_tageditordialog.h"

#include <fileref.h>
#include <tag.h>
#include <tstring.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QIntValidator>
#include <QKeySequence>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>

namespace {
  struct InitialValues {
    QString artist;
    QString album;
    QString title;
    QString year;
    QString track_number;
    QString genre;
    QString comment;
  };

  InitialValues read_initial(const Track &t) {
    InitialValues v;
    TagLib::FileRef f(QFile::encodeName(t.path()).constData(), false);
    if (!f.isNull() && f.tag()) {
      // Read raw tag values directly — Track::title() falls back to filename
      // when the tag is empty, which we don't want in an editor.
      auto *tag = f.tag();
      v.artist = QString::fromUtf8(tag->artist().toCString(true));
      v.album = QString::fromUtf8(tag->album().toCString(true));
      v.title = QString::fromUtf8(tag->title().toCString(true));
      v.genre = QString::fromUtf8(tag->genre().toCString(true));
      v.comment = QString::fromUtf8(tag->comment().toCString(true));
      if (tag->year() > 0) {
        v.year = QString::number(tag->year());
      }
      if (tag->track() > 0) {
        v.track_number = QString::number(tag->track());
      }
    }
    return v;
  }

  template<typename Getter>
  void apply_text(QLineEdit *edit, const QVector<InitialValues> &values, Getter g, const QString &multi_placeholder) {
    const QString first = g(values.first());
    for (int i = 1; i < values.size(); ++i) {
      if (g(values.at(i)) != first) {
        edit->setPlaceholderText(multi_placeholder);
        return;
      }
    }
    edit->setText(first);
  }

  void apply_plain(QPlainTextEdit *edit, const QVector<InitialValues> &values, const QString &multi_placeholder) {
    const QString first = values.first().comment;
    for (int i = 1; i < values.size(); ++i) {
      if (values.at(i).comment != first) {
        edit->setPlaceholderText(multi_placeholder);
        return;
      }
    }
    edit->setPlainText(first);
  }

  TagLib::String to_taglib(const QString &s) {
    const QByteArray utf8 = s.toUtf8();
    return TagLib::String(utf8.constData(), TagLib::String::UTF8);
  }
}

TagEditorDialog::TagEditorDialog(const QVector<Track> &tracks,
                                 std::shared_ptr<Playlist::Playlist> playlist,
                                 QWidget *parent)
    : QDialog(parent), ui(new Ui::TagEditorDialog), _tracks(tracks), _playlist(std::move(playlist)) {
  ui->setupUi(this);

  ui->lineEditYear->setValidator(new QIntValidator(0, 9999, this));
  ui->lineEditTrackNumber->setValidator(new QIntValidator(0, 9999, this));

  populate_fields();
  update_header();

  const bool navigable = _playlist && _tracks.size() == 1;
  ui->navContainer->setVisible(navigable);

  if (navigable) {
#ifdef Q_OS_MACOS
    const QKeySequence prevSeq(Qt::CTRL | Qt::Key_BracketLeft);
    const QKeySequence nextSeq(Qt::CTRL | Qt::Key_BracketRight);
#else
    const QKeySequence prevSeq(Qt::ALT | Qt::Key_Left);
    const QKeySequence nextSeq(Qt::ALT | Qt::Key_Right);
#endif
    ui->previousButton->setText(tr("Previous") + "  (" + prevSeq.toString(QKeySequence::NativeText) + ")");
    ui->nextButton->setText(tr("Next") + "  (" + nextSeq.toString(QKeySequence::NativeText) + ")");
    ui->previousButton->setAutoDefault(false);
    ui->nextButton->setAutoDefault(false);

    connect(ui->previousButton, &QPushButton::clicked, this, &TagEditorDialog::on_prevTrack);
    connect(ui->nextButton, &QPushButton::clicked, this, &TagEditorDialog::on_nextTrack);

    auto *prevShortcut = new QShortcut(prevSeq, this);
    auto *nextShortcut = new QShortcut(nextSeq, this);
    prevShortcut->setContext(Qt::WindowShortcut);
    nextShortcut->setContext(Qt::WindowShortcut);
    connect(prevShortcut, &QShortcut::activated, this, &TagEditorDialog::on_prevTrack);
    connect(nextShortcut, &QShortcut::activated, this, &TagEditorDialog::on_nextTrack);

    update_nav_state();
  }

  connect(ui->lineEditArtist, &QLineEdit::textEdited, this, [this](const QString &) { _artist.dirty = true; });
  connect(ui->lineEditAlbum, &QLineEdit::textEdited, this, [this](const QString &) { _album.dirty = true; });
  connect(ui->lineEditTitle, &QLineEdit::textEdited, this, [this](const QString &) { _title.dirty = true; });
  connect(ui->lineEditYear, &QLineEdit::textEdited, this, [this](const QString &) { _year.dirty = true; });
  connect(ui->lineEditTrackNumber, &QLineEdit::textEdited, this, [this](const QString &) { _track_number.dirty = true; });
  connect(ui->lineEditGenre, &QLineEdit::textEdited, this, [this](const QString &) { _genre.dirty = true; });
  // QPlainTextEdit has no textEdited; populate_fields() uses QSignalBlocker
  // so this connection is safe to make before or after the initial fill.
  connect(ui->plainTextComment, &QPlainTextEdit::textChanged, this, [this]() { _comment.dirty = true; });

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TagEditorDialog::on_save);
}

TagEditorDialog::~TagEditorDialog() {
  delete ui;
}

void TagEditorDialog::populate_fields() {
  // Suppress textChanged on the comment field so repopulation doesn't mark
  // _comment dirty on subsequent loads (during Previous/Next navigation).
  QSignalBlocker blocker(ui->plainTextComment);

  QVector<InitialValues> values;
  values.reserve(_tracks.size());
  for (const auto &t : _tracks) {
    values << read_initial(t);
  }

  const QString multi = tr("<multiple values>");

  apply_text(ui->lineEditArtist, values, [](const InitialValues &v) { return v.artist; }, multi);
  apply_text(ui->lineEditAlbum, values, [](const InitialValues &v) { return v.album; }, multi);
  apply_text(ui->lineEditTitle, values, [](const InitialValues &v) { return v.title; }, multi);
  apply_text(ui->lineEditYear, values, [](const InitialValues &v) { return v.year; }, multi);
  apply_text(ui->lineEditTrackNumber, values, [](const InitialValues &v) { return v.track_number; }, multi);
  apply_text(ui->lineEditGenre, values, [](const InitialValues &v) { return v.genre; }, multi);
  apply_plain(ui->plainTextComment, values, multi);
}

void TagEditorDialog::update_header() {
  if (_playlist && _tracks.size() == 1) {
    const int idx = _playlist->trackIndex(_tracks.first().uid());
    const int total = _playlist->tracks().size();
    if (idx >= 0) {
      ui->labelHeader->setText(tr("Track %1 of %2 — %3").arg(idx + 1).arg(total).arg(_tracks.first().formattedTitle()));
      return;
    }
  }
  ui->labelHeader->setText(tr("Editing %n track(s)", "", _tracks.size()));
}

void TagEditorDialog::update_nav_state() {
  if (!_playlist || _tracks.size() != 1) {
    ui->previousButton->setEnabled(false);
    ui->nextButton->setEnabled(false);
    return;
  }
  const auto tracks = _playlist->tracks();
  const int idx = _playlist->trackIndex(_tracks.first().uid());
  ui->previousButton->setEnabled(idx > 0);
  ui->nextButton->setEnabled(idx >= 0 && idx < tracks.size() - 1);
}

void TagEditorDialog::clear_dirty() {
  _artist.dirty = false;
  _album.dirty = false;
  _title.dirty = false;
  _year.dirty = false;
  _track_number.dirty = false;
  _genre.dirty = false;
  _comment.dirty = false;
}

void TagEditorDialog::load_track(int delta) {
  if (!_playlist || _tracks.size() != 1) {
    return;
  }
  const auto tracks = _playlist->tracks();
  const int idx = _playlist->trackIndex(_tracks.first().uid());
  if (idx < 0) {
    return;
  }
  const int newIdx = idx + delta;
  if (newIdx < 0 || newIdx >= tracks.size()) {
    return;
  }

  if (ui->autosaveCheckBox->isChecked()) {
    if (!commit_dirty()) {
      // Save failed; stay on the current track with the warning shown.
      return;
    }
  }

  _tracks.clear();
  _tracks << tracks.at(newIdx);
  clear_dirty();
  populate_fields();
  update_header();
  update_nav_state();
}

void TagEditorDialog::on_prevTrack() {
  load_track(-1);
}

void TagEditorDialog::on_nextTrack() {
  load_track(+1);
}

bool TagEditorDialog::save_one(const Track &t, QString *error) const {
  TagLib::FileRef f(QFile::encodeName(t.path()).constData(), false);
  if (f.isNull() || !f.tag()) {
    if (error) {
      *error = tr("Could not open file");
    }
    return false;
  }
  TagLib::Tag *tag = f.tag();

  if (_artist.dirty) {
    tag->setArtist(to_taglib(ui->lineEditArtist->text()));
  }
  if (_album.dirty) {
    tag->setAlbum(to_taglib(ui->lineEditAlbum->text()));
  }
  if (_title.dirty) {
    tag->setTitle(to_taglib(ui->lineEditTitle->text()));
  }
  if (_year.dirty) {
    tag->setYear(static_cast<unsigned int>(ui->lineEditYear->text().toUInt()));
  }
  if (_track_number.dirty) {
    tag->setTrack(static_cast<unsigned int>(ui->lineEditTrackNumber->text().toUInt()));
  }
  if (_genre.dirty) {
    tag->setGenre(to_taglib(ui->lineEditGenre->text()));
  }
  if (_comment.dirty) {
    tag->setComment(to_taglib(ui->plainTextComment->toPlainText()));
  }

  if (!f.save()) {
    if (error) {
      *error = tr("TagLib refused to save (file not writable?)");
    }
    return false;
  }
  return true;
}

bool TagEditorDialog::commit_dirty() {
  const bool any_dirty = _artist.dirty || _album.dirty || _title.dirty || _year.dirty
                         || _track_number.dirty || _genre.dirty || _comment.dirty;
  if (!any_dirty) {
    return true;
  }

  // TODO: move to QtConcurrent::run when large selections become common.
  QList<quint64> succeeded;
  QStringList failed;
  for (const auto &t : _tracks) {
    QString err;
    if (save_one(t, &err)) {
      succeeded << t.uid();
    } else {
      failed << QString("%1 (%2)").arg(t.path(), err);
    }
  }

  if (!failed.isEmpty()) {
    QMessageBox::warning(this, tr("Save failed"),
                         tr("Could not write tags for the following files:\n%1").arg(failed.join('\n')));
  }

  if (!succeeded.isEmpty()) {
    emit saved(succeeded);
  }

  if (failed.isEmpty()) {
    clear_dirty();
    return true;
  }
  return false;
}

void TagEditorDialog::on_save() {
  commit_dirty();
  accept();
}
