#include "trackinfodialog.h"
#include "ui_trackinfodialog.h"

#include <QMenu>
#include <QClipboard>
#include <QDebug>

TrackInfoDialog::TrackInfoDialog(const Track &track, QWidget *parent) : QDialog(parent), ui(new Ui::TrackInfoDialog) {
  ui->setupUi(this);
  setWindowTitle("Track info: " + track.formattedTitle());
  ui->tableView->horizontalHeader()->setVisible(false);
  ui->tableView->verticalHeader()->setVisible(false);
  ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->tableView->setModel(&model);
  setup_context_menu();
  setup_table(track);
}

TrackInfoDialog::~TrackInfoDialog() {
  delete ui;
}

void TrackInfoDialog::on_copy(const QPoint &pos) {
  auto index = ui->tableView->indexAt(pos);
  auto text = model.data(index);
  qApp->clipboard()->setText(text.toString());
}

void TrackInfoDialog::setup_table(const Track &track) {
  if (!track.artist().isEmpty()) {
    add_table_row("Artist", track.artist());
  }
  if (!track.album().isEmpty()) {
    add_table_row("Album", track.album());
  }
  if (!track.title().isEmpty()) {
    add_table_row("Title", track.title());
  }
  if (track.year() > 0) {
    add_table_row("Year", QString::number(track.year()));
  }
  if (!track.isStream()) {
    add_table_row("Track number", QString::number(track.track_number()));
    add_table_row("Duration", track.formattedDuration());
  }
  add_table_row("Format", track.format());
  add_table_row("Bitrate", QString::number(track.bitrate()));
  add_table_row("Sample rate", QString::number(track.sample_rate()));
  add_table_row("Channels", QString::number(track.channels()));
  if (track.isStream()) {
    add_table_row("Stream url", track.url().toString());
  } else {
    add_table_row("File path", track.path());
  }
  if (track.isCue()) {
    add_table_row("CUE start at", Track::formattedTime(track.begin()));
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
    QAction copy("Copy");
    connect(&copy, &QAction::triggered, [=]() {
      on_copy(pos);
    });
    menu.addAction(&copy);
    menu.exec(ui->tableView->viewport()->mapToGlobal(pos));
  });
}


