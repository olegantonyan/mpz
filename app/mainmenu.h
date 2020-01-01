#ifndef MAINMENU_H
#define MAINMENU_H

#include <QObject>
#include <QToolButton>
class MainMenu : public QObject {
  Q_OBJECT
public:
  explicit MainMenu(QToolButton *btn, QObject *parent = nullptr);

signals:
  void exit();

public slots:

private slots:
  void on_buttonClicked();

private:
  QToolButton *button;
};

#endif // MAINMENU_H
