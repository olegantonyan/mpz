#ifndef VOLUMEMENU_H
#define VOLUMEMENU_H

#include <QObject>
#include <QSlider>
#include <QMenu>
#include <QWidgetAction>
#include <QHBoxLayout>
#include <QPoint>

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

#endif // VOLUMEMENU_H
