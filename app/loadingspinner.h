#ifndef LOADINGSPINNER_H
#define LOADINGSPINNER_H

#include <QWidget>

class QTimer;

class LoadingSpinner : public QWidget {
  Q_OBJECT
public:
  explicit LoadingSpinner(QWidget *parent = nullptr, bool center_on_parent = true);

public slots:
  void start();
  void stop();

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  void updatePosition();

  QTimer *timer;
  int angle;
  bool center_on_parent;
};

#endif // LOADINGSPINNER_H
