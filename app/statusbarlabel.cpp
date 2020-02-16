#include "statusbarlabel.h"

#include <QDebug>
#include <QRegularExpression>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>

StatusBarLabel::StatusBarLabel(QStatusBar *sb, QWidget *parent) : QLabel(parent) {
  sb->addWidget(this);
  on_playerStopped();
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QLabel::customContextMenuRequested, this, &StatusBarLabel::on_contextMenu);
}

void StatusBarLabel::on_playerStopped() {
  _state = "Stopped";
  _stream_buffer = 0;
  setText(_state);
}

void StatusBarLabel::on_playerStarted(const Track &track) {
  _state = "Playing";
  auto t = _state + trackInfo(track);
  _stream_buffer = 0;
  setText(t);
}

void StatusBarLabel::on_playerPaused(const Track &track) {
  _state = "Paused";
  auto t = _state + trackInfo(track);
  setText(t);
}

void StatusBarLabel::on_streamBufferFill(const Track &track, int percents) {
  _stream_buffer = percents;
  if (_state != "Stopped") {
    setText(_state + trackInfo(track));
  }
}

void StatusBarLabel::on_progress(const Track &track, int current_seconds) {
  Q_UNUSED(current_seconds)
  setText(_state + trackInfo(track));
}

void StatusBarLabel::mouseDoubleClickEvent(QMouseEvent *event) {
  Q_UNUSED(event)
  emit doubleclicked();
  QLabel::mouseDoubleClickEvent(event);
}

void StatusBarLabel::on_contextMenu(const QPoint &pos) {
  QMenu menu;
  QAction copy_name("Copy");
  connect(&copy_name, &QAction::triggered, [=]() {
     qApp->clipboard()->setText(text());
  });
  menu.addAction(&copy_name);
  menu.exec(mapToGlobal(pos));
}

QString StatusBarLabel::trackInfo(const Track &t) const {
  auto c = ": " + t.shortText() + " | " + t.formattedAudioInfo();
  if (_stream_buffer > 0) {
    c += QString(" | stream buffer %1%").arg(_stream_buffer);
  }
  return c;
}
