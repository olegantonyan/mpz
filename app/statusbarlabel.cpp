#include "statusbarlabel.h"

StatusBarLabel::StatusBarLabel(QWidget *parent) : QLabel(parent) {
}

void StatusBarLabel::mouseDoubleClickEvent(QMouseEvent *event) {
  (void)event;
  emit doubleclicked();
}
