#include "busyspinner.h"

BusySpinner::BusySpinner(QWidget *_widget, QObject *parent) : QObject(parent), widget(_widget), show_count(0) {

  spinner = new LoadingSpinner(widget);

  spinner->start(); // gets the show on the road!
  spinner->setToolTip(tr("Background operation running"));

  hide();
}

void BusySpinner::show() {
  widget->show();
  show_count++;
}

void BusySpinner::hide() {
  if (show_count > 0) {
    show_count--;
  }
  if (show_count == 0) {
    widget->hide();
  }
}
