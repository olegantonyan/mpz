#ifndef STATUSBARLABEL_H
#define STATUSBARLABEL_H

#include <QObject>
#include <QLabel>

class StatusBarLabel : public QLabel {
  Q_OBJECT

public:
  explicit StatusBarLabel(QWidget *parent = nullptr);

signals:
  void doubleclicked();

  // QWidget interface
protected:
  void mouseDoubleClickEvent(QMouseEvent *event);
};

#endif // STATUSBARLABEL_H
