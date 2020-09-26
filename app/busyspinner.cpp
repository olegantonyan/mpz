#include "busyspinner.h"

BusySpinner::BusySpinner(QWidget *_widget, QObject *parent) : QObject(parent), widget(_widget), show_count(0) {

  spinner = new WaitingSpinnerWidget(widget);

  spinner->setRoundness(70.0);
  spinner->setMinimumTrailOpacity(15.0);
  spinner->setTrailFadePercentage(70.0);
  spinner->setNumberOfLines(7);
  spinner->setLineLength(6);
  spinner->setLineWidth(3);
  spinner->setInnerRadius(3);
  spinner->setRevolutionsPerSecond(1);
  spinner->setColor(QColor(120, 120, 120));

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
