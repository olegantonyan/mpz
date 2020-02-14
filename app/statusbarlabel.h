#ifndef STATUSBARLABEL_H
#define STATUSBARLABEL_H

#include "track.h"

#include <QObject>
#include <QLabel>
#include <QStatusBar>

class StatusBarLabel : public QLabel {
  Q_OBJECT
public:
  explicit StatusBarLabel(QStatusBar *sb, QWidget *parent = nullptr);

public slots:
  void on_playerStopped();
  void on_playerStarted(const Track &track);
  void on_playerPaused(const Track &track);
  void on_streamBufferFill(const Track &track, int percents);

signals:
  void doubleclicked();

protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // STATUSBARLABEL_H
