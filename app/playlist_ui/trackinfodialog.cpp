#include "trackinfodialog.h"
#include "ui_trackinfodialog.h"

#include "config/global.h"
#include "lyrics/lrclibclient.h"
#include "lyrics/lrcparser.h"

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

#include <QMenu>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QPixmap>
#include <QPointer>

TrackInfoDialog::TrackInfoDialog(const Track &track, QWidget *parent) : QDialog(parent), ui(new Ui::TrackInfoDialog) {
  ui->setupUi(this);

  setWindowTitle(windowTitle() + ": " + track.formattedTitle());
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
  track_dir = track.dir();
  cover_art_path = track.artCover();
  if (!cover_art_path.isEmpty()) {
    ui->labelCoverArt->setContextMenuPolicy(Qt::CustomContextMenu);
  }
  ui->toolButtonOpenFileManager->setVisible(!track.isMpd());
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

  connect(ui->tableView, &QTableView::customContextMenuRequested, [=](const QPoint &pos) {
    if(!ui->tableView->indexAt(pos).isValid()) {
      return;
    }
    QMenu menu;
    QAction copy(tr("Copy"));
    connect(&copy, &QAction::triggered, [=]() {
      on_copy(pos);
    });
    QAction search(tr("Search on web"));
    connect(&search, &QAction::triggered, [=]() {
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
  QDesktopServices::openUrl(QUrl::fromLocalFile(track_dir));
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

  if (providers.contains("lrclib") && !track.artist().isEmpty() && !track.title().isEmpty()) {
    render_lyrics_state(tr("Searching lyrics..."));
    auto *client = new Lyrics::LrcLibClient(this);
    QPointer<TrackInfoDialog> guard(this);
    connect(client, &Lyrics::LrcLibClient::found, this, [guard, client](const QString &lyrics) {
      if (guard) {
        guard->render_lyrics(guard->tr("LRCLIB"), lyrics);
      }
      client->deleteLater();
    });
    connect(client, &Lyrics::LrcLibClient::notFound, this, [guard, client]() {
      if (guard) {
        guard->render_lyrics_state(guard->tr("No lyrics found."));
      }
      client->deleteLater();
    });
    connect(client, &Lyrics::LrcLibClient::failed, this, [guard, client](const QString &msg) {
      if (guard) {
        qWarning() << "lrclib error:" << msg;
        guard->render_lyrics_state(guard->tr("No lyrics found."));
      }
      client->deleteLater();
    });
    client->fetch(track.artist(), track.title(), track.album(), static_cast<int>(track.duration() / 1000));
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
  connect(&copy, &QAction::triggered, [=]() {
    QPixmap pixmap(cover_art_path);
    if (!pixmap.isNull()) {
      QApplication::clipboard()->setPixmap(pixmap);
    }
  });

  QAction show_in_filemanager(tr("Open in external viewer"));
  show_in_filemanager.setIcon(ui->labelCoverArt->style()->standardIcon(QStyle::SP_DirLinkIcon));
  connect(&show_in_filemanager, &QAction::triggered, [=]() {
    if (!cover_art_path.isEmpty()) {
      QDesktopServices::openUrl(QUrl::fromLocalFile(cover_art_path));
    }
  });
  menu.addAction(&copy);
  menu.addAction(&show_in_filemanager);
  menu.exec(ui->labelCoverArt->mapToGlobal(pos));
}
