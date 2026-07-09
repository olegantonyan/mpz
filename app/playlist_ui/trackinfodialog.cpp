#include "trackinfodialog.h"
#include "ui_trackinfodialog.h"

#include "icons.h"
#include "tageditordialog.h"
#include "config/global.h"
#include "lyrics/lrcparser.h"
#include "lyrics/providerchain.h"
#include "reveal_in_filemanager.h"

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

#include <QMenu>
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QPixmap>

TrackInfoDialog::TrackInfoDialog(const Track &track, std::shared_ptr<Playlist::Playlist> playlist, QWidget *parent) :
  QDialog(parent), ui(new Ui::TrackInfoDialog), _track(track), _playlist(playlist) {
  ui->setupUi(this);

  base_title = windowTitle();
  setWindowTitle(base_title + ": " + track.formattedTitle());
  ui->tableView->horizontalHeader()->setVisible(false);
  ui->tableView->verticalHeader()->setVisible(false);
  ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->tableView->setModel(&model);
  setup_context_menu();
  setup_table(track);
  setup_cover_art(track);
  setup_lyrics(track);
  ui->splitter->setSizes({920, 300});
  ui->metadataSplitter->setSizes({600, 320});
  track_path = track.path();
  cover_art_path = track.artCover();
  if (!cover_art_path.isEmpty()) {
    ui->labelCoverArt->setContextMenuPolicy(Qt::CustomContextMenu);
  }
  ui->toolButtonOpenFileManager->setVisible(!track.isMpd());
  ui->toolButtonEditTags->setVisible(!track.isCue() && !track.isMpd() && !track.isStream());
}

TrackInfoDialog::~TrackInfoDialog() {
  delete ui;
}

void TrackInfoDialog::on_copy(const QPoint &pos) {
  auto index = ui->tableView->indexAt(pos);
  auto text = model.data(index);
  qApp->clipboard()->setText(text.toString());
}

void TrackInfoDialog::on_search(const QPoint &pos) {
  auto index = ui->tableView->indexAt(pos);
  auto text = model.data(index).toString();
  auto term = QString("https://duckduckgo.com/?q=%1").arg(QString(QUrl::toPercentEncoding(text)));
  QDesktopServices::openUrl(QUrl(term));
}

void TrackInfoDialog::setup_table(const Track &track) {
  if (!track.artist().isEmpty()) {
    add_table_row(tr("Artist"), track.artist());
  }
  if (!track.album().isEmpty()) {
    add_table_row(tr("Album"), track.album());
  }
  if (!track.title().isEmpty()) {
    add_table_row(tr("Title"), track.title());
  }
  if (track.year() > 0) {
    add_table_row(tr("Year"), QString::number(track.year()));
  }
  if (!track.isStream()) {
    add_table_row(tr("Track number"), QString::number(track.track_number()));
    add_table_row(tr("Duration"), track.formattedDuration());
  }
  add_table_row(tr("Format"), track.format());
  if (track.bitrate() > 0) {
    add_table_row(tr("Bitrate"), QString::number(track.bitrate()));
  }
  if (track.sample_rate() > 0) {
    add_table_row(tr("Sample rate"), QString::number(track.sample_rate()));
  }
  if (track.channels() > 0) {
    add_table_row(tr("Channels"), QString::number(track.channels()));
  }
  if (track.isStream()) {
    add_table_row(tr("Stream url"), track.url().toString());
  }
  if (!track.path().isEmpty()) {
    if (track.isMpd()) {
      auto url = track.mpd_server_url();
      if (!url.password().isEmpty()) {
        url.setPassword("***");
      }
      add_table_row(tr("File path"), url.toString() + "/" + track.path());
    } else {
      add_table_row(tr("File path"), track.path());
    }
  }
  if (track.isCue()) {
    add_table_row(tr("CUE start at"), Track::formattedTime(track.begin()));
  }

  ui->tableView->resizeColumnsToContents();
  ui->tableView->resizeRowsToContents();
}

void TrackInfoDialog::add_table_row(const QString &title, const QString &content) {
  QList<QStandardItem *> row;
  row.append(new QStandardItem(title));
  row.append(new QStandardItem(content));
  model.appendRow(row);
}

void TrackInfoDialog::setup_context_menu() {
  ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(ui->tableView, &QTableView::customContextMenuRequested, this, [=](const QPoint &pos) {
    if(!ui->tableView->indexAt(pos).isValid()) {
      return;
    }
    QMenu menu;
    QAction copy(tr("Copy"));
    connect(&copy, &QAction::triggered, this, [=]() {
      on_copy(pos);
    });
    QAction search(tr("Search on web"));
    connect(&search, &QAction::triggered, this, [=]() {
      on_search(pos);
    });
    menu.addAction(&copy);
    menu.addAction(&search);
    menu.exec(ui->tableView->viewport()->mapToGlobal(pos));
  });
}

void TrackInfoDialog::setup_cover_art(const Track &track) {
  auto path = track.artCover();
  if (path.isEmpty()) {
    return;
  }
  QPixmap cover(path);
  if (cover.isNull()) {
    return;
  }
  ui->labelCoverArt->setPixmap(cover.scaledToHeight(height()));
}

void TrackInfoDialog::on_toolButtonOpenFileManager_clicked() {
  revealInFileManager({ track_path });
}

void TrackInfoDialog::on_toolButtonEditTags_clicked() {
  TagEditorDialog *dlg = new TagEditorDialog({_track}, _playlist);
  dlg->setModal(false);
  connect(dlg, &TagEditorDialog::finished, dlg, &TagEditorDialog::deleteLater);
  // Queued: the playlist reloads tracks in response to saved() (via tracksChanged), refresh must run after.
  connect(dlg, &TagEditorDialog::saved, this, &TrackInfoDialog::refresh_track, Qt::QueuedConnection);
  emit tagEditorOpened(dlg);
  dlg->show();
}

void TrackInfoDialog::refresh_track(const QList<quint64> &uids) {
  if (!_playlist || !uids.contains(_track.uid())) {
    return;
  }
  const Track t = _playlist->trackBy(_track.uid());
  if (t.uid() != _track.uid()) {
    return;
  }
  _track = t;
  model.removeRows(0, model.rowCount());
  setup_table(_track);
  setWindowTitle(base_title + ": " + _track.formattedTitle());
}

void TrackInfoDialog::setup_lyrics(const Track &track) {
  if (track.isStream()) {
    ui->lyricsWidget->setVisible(false);
    return;
  }

  Config::Global global;
  const auto providers = global.lyricsProviders();

  for (const auto &name : providers) {
    QString text;
    if (name == "embedded") {
      text = fetch_embedded_lyrics(track);
    } else if (name == "sidecar") {
      text = fetch_sidecar_lyrics(track);
    } else {
      continue;
    }
    if (!text.isEmpty()) {
      render_lyrics(name, text);
      return;
    }
  }

  QStringList online;
  const auto known = Lyrics::ProviderChain::knownProviders();
  for (const auto &name : providers) {
    if (known.contains(name)) {
      online << name;
    }
  }
  if (!online.isEmpty() && !track.artist().isEmpty() && !track.title().isEmpty()) {
    render_lyrics_state(tr("Searching lyrics..."));
    auto *chain = new Lyrics::ProviderChain(this);
    connect(chain, &Lyrics::ProviderChain::found, this, [this, chain](const QString &provider, const QString &lyrics) {
      render_lyrics(Lyrics::ProviderChain::displayName(provider), lyrics);
      chain->deleteLater();
    });
    connect(chain, &Lyrics::ProviderChain::notFound, this, [this, chain]() {
      render_lyrics_state(tr("No lyrics found."));
      chain->deleteLater();
    });
    chain->fetch(online, Lyrics::TrackQuery{track.artist(), track.title(), track.album(),
                                            static_cast<int>(track.duration() / 1000)});
    return;
  }

  render_lyrics_state(tr("No lyrics found."));
}

QString TrackInfoDialog::fetch_embedded_lyrics(const Track &track) const {
  if (track.isMpd() || track.isCue() || track.path().isEmpty() || !QFile::exists(track.path())) {
    return QString();
  }
  TagLib::FileRef f(track.path().toUtf8().constData());
  if (f.isNull() || !f.tag()) {
    return QString();
  }
  auto props = f.tag()->properties();
  auto it = props.find("LYRICS");
  if (it == props.end() || it->second.isEmpty()) {
    return QString();
  }
  return QString::fromUtf8(it->second.front().toCString(true));
}

QString TrackInfoDialog::fetch_sidecar_lyrics(const Track &track) const {
  if (track.isMpd() || track.path().isEmpty()) {
    return QString();
  }
  QFileInfo fi(track.path());
  for (const QString &ext : { QStringLiteral("lrc"), QStringLiteral("txt") }) {
    QString candidate = fi.path() + "/" + fi.completeBaseName() + "." + ext;
    if (!QFile::exists(candidate)) {
      continue;
    }
    QFile f(candidate);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      return QString::fromUtf8(f.readAll());
    }
  }
  return QString();
}

void TrackInfoDialog::render_lyrics(const QString &source, const QString &raw) {
  ui->labelLyricsSource->setText(QString("(%1)").arg(source));
  const QString text = Lyrics::LrcParser::looksLikeLrc(raw)
                         ? Lyrics::LrcParser::stripTimestamps(raw)
                         : raw.trimmed();
  ui->plainTextLyrics->setPlainText(text);
}

void TrackInfoDialog::render_lyrics_state(const QString &message) {
  ui->labelLyricsSource->setText(QString());
  ui->plainTextLyrics->clear();
  ui->plainTextLyrics->setPlaceholderText(message);
}

void TrackInfoDialog::on_labelCoverArt_customContextMenuRequested(const QPoint &pos) {
  QMenu menu;
  QAction copy(tr("Copy to clipboard"));
  connect(&copy, &QAction::triggered, this, [=]() {
    QPixmap pixmap(cover_art_path);
    if (!pixmap.isNull()) {
      QApplication::clipboard()->setPixmap(pixmap);
    }
  });

  QAction show_in_filemanager(tr("Open in external viewer"));
  show_in_filemanager.setIcon(Icons::get(Icons::Icon::FolderReveal));
  connect(&show_in_filemanager, &QAction::triggered, this, [=]() {
    if (!cover_art_path.isEmpty()) {
      QDesktopServices::openUrl(QUrl::fromLocalFile(cover_art_path));
    }
  });
  menu.addAction(&copy);
  menu.addAction(&show_in_filemanager);
  menu.exec(ui->labelCoverArt->mapToGlobal(pos));
}
