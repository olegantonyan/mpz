#include "trackinfodialog.h"
#include "ui_trackinfodialog.h"

#include <QMenu>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QPixmap>

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
  track_dir = track.dir();
  cover_art_path = track.artCover();
  if (!cover_art_path.isEmpty()) {
    ui->labelCoverArt->setContextMenuPolicy(Qt::CustomContextMenu);
  }
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
  add_table_row(tr("Bitrate"), QString::number(track.bitrate()));
  add_table_row(tr("Sample rate"), QString::number(track.sample_rate()));
  add_table_row(tr("Channels"), QString::number(track.channels()));
  if (track.isStream()) {
    add_table_row(tr("Stream url"), track.url().toString());
  }
  if (!track.path().isEmpty()) {
    add_table_row(tr("File path"), track.path());
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

