#ifndef SLIDINGBANNER_H
#define SLIDINGBANNER_H

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

class SlidingBanner : public QWidget {
  Q_OBJECT
public:
  enum BannerType { Success, Error };

  explicit SlidingBanner(QWidget *parent = nullptr);

public slots:
  void showMessage(const QString &text, BannerType type = Success, int timeoutMs = 0);
  void expand();
  void collapse();

private:
  QLabel *label;
  QPropertyAnimation *animation;
  const int bannerHeight = 40;
  QTimer timer;
};


#endif // SLIDINGBANNER_H
