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
  void on_progress(const Track &track, int current_seconds);

signals:
  void doubleclicked();

protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
  QString trackInfo(const Track &t) const;

  QString _state;
  int _stream_buffer;
};

#endif // STATUSBARLABEL_H
