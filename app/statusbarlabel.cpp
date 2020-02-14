#include "statusbarlabel.h"

#include <QDebug>
#include <QRegularExpression>

StatusBarLabel::StatusBarLabel(QStatusBar *sb, QWidget *parent) : QLabel(parent) {
  sb->addWidget(this);
  on_playerStopped();
}

void StatusBarLabel::on_playerStopped() {
  setText("Stopped");
}

void StatusBarLabel::on_playerStarted(const Track &track) {
  auto t = QString("Playing ") + track.filename() + " | " + track.formattedAudioInfo();
  setText(t);
}

void StatusBarLabel::on_playerPaused(const Track &track) {
  auto t = QString("Paused ") + track.filename() + " | " + track.formattedAudioInfo();
  setText(t);
}

void StatusBarLabel::on_streamBufferFill(const Track &track, int percents) {
  Q_UNUSED(track)

  if (!text().contains("Stopped") && percents >=0) {
    if (text().contains(" | stream buffer")) {
      QRegularExpression regex(".+\\| stream buffer (\\d+%)");
      auto m = regex.match(text());
      if (m.hasMatch()) {
        setText(text().replace(m.captured(1), QString("%1%").arg(percents)));
      }
    } else {
      setText(text() + QString(" | stream buffer %1%").arg(percents));
    }
  }
}

void StatusBarLabel::mouseDoubleClickEvent(QMouseEvent *event) {
  Q_UNUSED(event)
  emit doubleclicked();
  QLabel::mouseDoubleClickEvent(event);
}
