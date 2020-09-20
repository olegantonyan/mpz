#include "playback_log_ui/playbacklogdialog.h"
#include "ui_playbacklogdialog.h"

#include <QMenu>
#include <QAction>
#include <QClipboard>

PlaybackLogDialog::PlaybackLogDialog(PlaybackLogUi::Model *m, QWidget *parent) : QDialog(parent), ui(new Ui::PlaybackLogDialog), model(m) {
  ui->setupUi(this);
  setWindowTitle("Playback log");
  ui->tableView->horizontalHeader()->setVisible(false);
  ui->tableView->verticalHeader()->setVisible(false);
  ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->tableView->setModel(model);
  ui->tableView->resizeColumnsToContents();

  connect(model, &PlaybackLogUi::Model::changed, ui->tableView, &QTableView::resizeColumnsToContents);

  setup_context_menu();
}

PlaybackLogDialog::~PlaybackLogDialog() {
  delete ui;
}

void PlaybackLogDialog::on_copy(const QPoint &pos) {
  auto index = ui->tableView->indexAt(pos);
  auto text = model->data(index);
  qApp->clipboard()->setText(text.toString());
}

void PlaybackLogDialog::on_jump_to(const QModelIndex &idx) {
  auto track_uid = model->itemAt(idx).track_uid;
  emit jump_to_track(track_uid);
}

void PlaybackLogDialog::setup_context_menu() {
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
    QAction jump_to("Jump to");
    connect(&jump_to, &QAction::triggered, [=]() {
      auto index = ui->tableView->indexAt(pos);
      on_jump_to(index);
    });

    menu.addAction(&copy);
    menu.addAction(&jump_to);
    menu.exec(ui->tableView->viewport()->mapToGlobal(pos));
  });

  connect(ui->tableView, &QTableView::doubleClicked, this, &PlaybackLogDialog::on_jump_to);
}
