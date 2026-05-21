#include "tageditordialog.h"
#include "ui_tageditordialog.h"

#include <fileref.h>
#include <tag.h>
#include <tstring.h>

#include <QDialogButtonBox>
#include <QFile>
#include <QIntValidator>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>

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

TagEditorDialog::TagEditorDialog(const QVector<Track> &tracks, QWidget *parent)
    : QDialog(parent), ui(new Ui::TagEditorDialog), _tracks(tracks) {
  ui->setupUi(this);

  ui->lineEditYear->setValidator(new QIntValidator(0, 9999, this));
  ui->lineEditTrackNumber->setValidator(new QIntValidator(0, 9999, this));

  ui->labelHeader->setText(tr("Editing %n track(s)", "", _tracks.size()));

  populate_fields();

  connect(ui->lineEditArtist, &QLineEdit::textEdited, this, [this](const QString &) { _artist.dirty = true; });
  connect(ui->lineEditAlbum, &QLineEdit::textEdited, this, [this](const QString &) { _album.dirty = true; });
  connect(ui->lineEditTitle, &QLineEdit::textEdited, this, [this](const QString &) { _title.dirty = true; });
  connect(ui->lineEditYear, &QLineEdit::textEdited, this, [this](const QString &) { _year.dirty = true; });
  connect(ui->lineEditTrackNumber, &QLineEdit::textEdited, this, [this](const QString &) { _track_number.dirty = true; });
  connect(ui->lineEditGenre, &QLineEdit::textEdited, this, [this](const QString &) { _genre.dirty = true; });
  // QPlainTextEdit has no textEdited; textChanged also fires from setPlainText
  // during construction, so wire it AFTER populate_fields has run.
  connect(ui->plainTextComment, &QPlainTextEdit::textChanged, this, [this]() { _comment.dirty = true; });

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TagEditorDialog::on_save);
}

TagEditorDialog::~TagEditorDialog() {
  delete ui;
}

void TagEditorDialog::populate_fields() {
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

void TagEditorDialog::on_save() {
  const bool any_dirty = _artist.dirty || _album.dirty || _title.dirty || _year.dirty
                         || _track_number.dirty || _genre.dirty || _comment.dirty;
  if (!any_dirty) {
    accept();
    return;
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
  accept();
}
