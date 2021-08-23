#ifndef DIRECTORYSORTMENU_H
#define DIRECTORYSORTMENU_H

#include <QObject>
#include <QToolButton>
#include <QAction>

namespace DirectoryUi {
  class SortMenu : public QObject {
    Q_OBJECT
  public:
    explicit SortMenu(QToolButton *button);

  signals:
    void triggered(const QString& criteria);

  private slots:
    void on_open();

  private:
    QToolButton *button;
  };
}

#endif // DIRECTORYSORTMENU_H
