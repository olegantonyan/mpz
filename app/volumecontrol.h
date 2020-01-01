#ifndef VOLUMECONTROL_H
#define VOLUMECONTROL_H

#include <QObject>
#include <QToolButton>
#include <QSlider>
#include <QMenu>
#include <QWidgetAction>
#include <QHBoxLayout>
#include <QPoint>
#include <QEvent>

class VolumeMenu : public QObject {
  Q_OBJECT
public:
  explicit VolumeMenu(QObject *parent = nullptr);
  QSize sizeHint() const;

public slots:
  void show(const QPoint& pos);
  void setValue(int value);

signals:
  void changed(int value);

private:
  QWidgetAction action;
  QWidget widget;
  QMenu menu;
  QSlider slider;
  QHBoxLayout layout;
};

class VolumeControl : public QObject {
  Q_OBJECT
public:
  explicit VolumeControl(QToolButton *btn, int initial_value, QObject *parent = nullptr);

signals:
  void changed(int value);
  void increased(int by);
  void decreased(int by);

public slots:
  void setValue(int value);

private slots:
  void on_buttonClicked();

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  QToolButton *button;
  VolumeMenu menu;
};

#endif // VOLUMECONTROL_H
