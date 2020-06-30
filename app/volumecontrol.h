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
#include <QObject>

namespace PrivateVolumeControl {
  class Menu : public QObject {
    Q_OBJECT
  public:
    explicit Menu(QObject *parent = nullptr);
    QSize sizeHint() const;
    int value() const;

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
}

class VolumeControl : public QObject {
  Q_OBJECT
public:
  explicit VolumeControl(QToolButton *btn, int initial_value, QObject *parent = nullptr);

signals:
  void changed(int value);

public slots:
  void setValue(int value);

private slots:
  void on_buttonClicked();

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  QToolButton *button;
  PrivateVolumeControl::Menu menu;
};

#endif // VOLUMECONTROL_H
