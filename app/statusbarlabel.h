#ifndef STATUSBARLABEL_H
#define STATUSBARLABEL_H

#include "track.h"

#include <QObject>
#include <QLabel>
#include <QStatusBar>
#include <QPoint>

class StatusBarLabel : public QLabel {
  Q_OBJECT
public:
  explicit StatusBarLabel(QWidget *parent = nullptr);

public slots:
  void on_playerStopped();
  void on_playerStarted(const Track &track);
  void on_playerPaused(const Track &track);
  void on_streamBufferFill(const Track &track, quint32 bytes);
  void on_progress(const Track &track, int current_seconds);

signals:
  void doubleclicked();
  void showPlaybackLog();

protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
  void on_contextMenu(const QPoint &pos);

private:
  QString trackInfo(const Track &t) const;

  QString _state;
  quint32 _stream_buffer;
  QString humanized_bytes(quint32 bytes) const;
  QString trackTitle() const;
};

#endif // STATUSBARLABEL_H
