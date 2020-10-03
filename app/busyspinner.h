#ifndef BUSYSPINNER_H
#define BUSYSPINNER_H

#include "waitingspinnerwidget.h"

#include <QObject>
#include <QWidget>

class BusySpinner : public QObject {
  Q_OBJECT
public:
  explicit BusySpinner(QWidget *_widget, QObject *parent = nullptr);

signals:

public slots:
  void show();
  void hide();

private:
  QWidget *widget;
  WaitingSpinnerWidget *spinner;
  quint16 show_count;
};

#endif // BUSYSPINNER_H
