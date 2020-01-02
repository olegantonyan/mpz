#include "statusbarlabel.h"

StatusBarLabel::StatusBarLabel(QStatusBar *sb, QWidget *parent) : QLabel(parent) {
  sb->addWidget(this);
  on_playerStopped();
}

void StatusBarLabel::on_playerStopped() {
  setText("Stopped");
}

void StatusBarLabel::on_playerStarted(const Track &track) {
  setText(QString("Playing ") + track.filename() + " | " + track.formattedAudioInfo());
}

void StatusBarLabel::on_playerPaused(const Track &track) {
  setText(QString("Paused ") + track.filename() + " | " + track.formattedAudioInfo());
}

void StatusBarLabel::mouseDoubleClickEvent(QMouseEvent *event) {
  Q_UNUSED(event)
  emit doubleclicked();
  QLabel::mouseDoubleClickEvent(event);
}
