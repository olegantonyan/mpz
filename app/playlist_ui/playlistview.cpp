#include "playlistview.h"

#include <QDebug>
#include <QHeaderView>
#include <QTimer>

namespace PlaylistUi {
  View::View(QTableView *v, QObject *parent) : QObject(parent) {
    view = v;
    model = new Model(this);
    view->setModel(model);
    view->horizontalHeader()->hide();
    view->verticalHeader()->hide();
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setShowGrid(false);
    //view->setFocusPolicy(Qt::NoFocus);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //view->horizontalHeader()->setStretchLastSection(true);

    for (int c = 0; c < view->horizontalHeader()->count(); ++c) {
      view->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    }

    //view->setColumnWidth(0, 400);

   // qDebug() << view->columnWidth(0);

    /*
    QTimer::singleShot(100, [=]() {

      int total_width = 0;
      for (int i = 0; i < 5; i++) {
        total_width += view->columnWidth(i);
      }
      qDebug() << view->geometry().width();
      qDebug() << total_width;
      total_width = view->geometry().width() - 50;
      view->setColumnWidth(0, total_width * 0.3);
      view->setColumnWidth(1, total_width * 0.3);
      view->setColumnWidth(2, total_width * 0.3);
      view->setColumnWidth(3, total_width * 0.05);
      view->setColumnWidth(4, total_width * 0.05);
    });
*/
    view->installEventFilter(&interceptor);
  }

  void View::on_load(const std::shared_ptr<Playlist> pi) {
    model->setTracks(pi->tracks());
  }

  void View::on_unload() {
    model->setTracks(QVector<Track>());
  }

  bool ResizeEventInterceptor::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::Resize) {
      QTableView *view = dynamic_cast<QTableView *>(obj);
      int total_width = view->width() - 50;
      view->setColumnWidth(0, total_width * 0.3);
      view->setColumnWidth(1, total_width * 0.3);
      view->setColumnWidth(2, total_width * 0.3);
      view->setColumnWidth(3, total_width * 0.05);
      view->setColumnWidth(4, total_width * 0.05);
    }
    return QObject::eventFilter(obj, event);
  }
}
