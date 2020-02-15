#include "statusbarlabel.h"

#include <QDebug>
#include <QRegularExpression>

StatusBarLabel::StatusBarLabel(QStatusBar *sb, QWidget *parent) : QLabel(parent) {
  sb->addWidget(this);
  on_playerStopped();
}

void StatusBarLabel::on_playerStopped() {
  _state = "Stopped";
  _stream_buffer = 0;
  setText(_state);
}

void StatusBarLabel::on_playerStarted(const Track &track) {
  _state = "Playing";
  auto t = _state + trackInfo(track);
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
  if (track.isStream()) {
    setText(_state + trackInfo(track));
  }
}

void StatusBarLabel::mouseDoubleClickEvent(QMouseEvent *event) {
  Q_UNUSED(event)
  emit doubleclicked();
  QLabel::mouseDoubleClickEvent(event);
}

QString StatusBarLabel::trackInfo(const Track &t) const {
  if (t.isStream()) {
    return " " + t.streamMeta().stream() + QString(" | stream buffer %1%").arg(_stream_buffer);
  } else {
    return " " + t.filename() + " | " + t.formattedAudioInfo();
  }
}
