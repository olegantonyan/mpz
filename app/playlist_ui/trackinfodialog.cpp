#include "trackinfodialog.h"
#include "ui_trackinfodialog.h"

#include <QMenu>
#include <QClipboard>

TrackInfoDialog::TrackInfoDialog(const Track &t, QWidget *parent) : QDialog(parent), ui(new Ui::TrackInfoDialog), track(t) {
  ui->setupUi(this);
  setWindowTitle("Track " + track.formattedTitle());

  ui->tableView->setModel(&model);

  ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->tableView, &QTableView::customContextMenuRequested, [=](const QPoint &pos) {
    if(!ui->tableView->indexAt(pos).isValid()) {
      return;
    }
    QMenu menu;
    QAction copy("Copy");
    connect(&copy, &QAction::triggered, [=]() {
      auto index = ui->tableView->indexAt(pos);
      auto text = model.data(index);
      qApp->clipboard()->setText(text.toString());
    });
    menu.addAction(&copy);
    menu.exec(ui->tableView->viewport()->mapToGlobal(pos));
  });

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
  model.setItem(row, 1, new QStandardItem(track.track_number()));
  row++;

  model.setItem(row, 0, new QStandardItem("Duration"));
  model.setItem(row, 1, new QStandardItem(track.formattedDuration()));
  row++;

  model.setItem(row, 0, new QStandardItem("Audio info"));
  model.setItem(row, 1, new QStandardItem(track.formattedAudioInfo()));
  row++;

  model.setItem(row, 0, new QStandardItem("File path"));
  model.setItem(row, 1, new QStandardItem(track.path()));
  row++;

  model.setItem(row, 0, new QStandardItem("Stream url"));
  model.setItem(row, 1, new QStandardItem(track.url().toString()));
  row++;

  ui->tableView->resizeColumnsToContents();
}

TrackInfoDialog::~TrackInfoDialog() {
  delete ui;
}
