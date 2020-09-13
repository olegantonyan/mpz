#include "trackinfodialog.h"
#include "ui_trackinfodialog.h"

#include <QMenu>
#include <QClipboard>
#include <QDebug>

TrackInfoDialog::TrackInfoDialog(const Track &track, QWidget *parent) : QDialog(parent), ui(new Ui::TrackInfoDialog) {
  ui->setupUi(this);
  setWindowTitle(track.formattedTitle());
  ui->tableView->horizontalHeader()->setVisible(false);
  ui->tableView->verticalHeader()->setVisible(false);
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
  int row = 0;

  model.setItem(row, 0, new QStandardItem("Artist"));
  model.setItem(row, 1, new QStandardItem(track.artist()));
  row++;

  model.setItem(row, 0, new QStandardItem("Album"));
  model.setItem(row, 1, new QStandardItem(track.album()));
  row++;

  model.setItem(row, 0, new QStandardItem("Title"));
  model.setItem(row, 1, new QStandardItem(track.title()));
  row++;

  model.setItem(row, 0, new QStandardItem("Year"));
  model.setItem(row, 1, new QStandardItem(track.year()));
  row++;

  model.setItem(row, 0, new QStandardItem("Track number"));
  model.setItem(row, 1, new QStandardItem(QString::number(track.track_number())));
  row++;

  model.setItem(row, 0, new QStandardItem("Duration"));
  model.setItem(row, 1, new QStandardItem(track.formattedDuration()));
  row++;

  model.setItem(row, 0, new QStandardItem("Format"));
  model.setItem(row, 1, new QStandardItem(track.format()));
  row++;

  model.setItem(row, 0, new QStandardItem("Bitrate"));
  model.setItem(row, 1, new QStandardItem(QString::number(track.bitrate())));
  row++;

  model.setItem(row, 0, new QStandardItem("Sample rate"));
  model.setItem(row, 1, new QStandardItem(QString::number(track.sample_rate())));
  row++;

  model.setItem(row, 0, new QStandardItem("Channels"));
  model.setItem(row, 1, new QStandardItem(QString::number(track.channels())));
  row++;

  if (track.isStream()) {
    model.setItem(row, 0, new QStandardItem("Stream url"));
    model.setItem(row, 1, new QStandardItem(track.url().toString()));
    row++;
  } else {
    model.setItem(row, 0, new QStandardItem("File path"));
    model.setItem(row, 1, new QStandardItem(track.path()));
    row++;
  }

  if (track.isCue()) {
    model.setItem(row, 0, new QStandardItem("CUE start"));
    model.setItem(row, 1, new QStandardItem(QString::number(track.begin())));
    row++;
  }

  ui->tableView->resizeColumnsToContents();
  ui->tableView->resizeRowsToContents();
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
