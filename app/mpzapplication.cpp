#include "mpzapplication.h"

#include <QEvent>
#include <QFileOpenEvent>
#include <QUrl>

MpzApplication::MpzApplication(int &argc, char **argv) : QApplication(argc, argv) {
}

QStringList MpzApplication::drainPendingFiles() {
  QStringList files = pending_files;
  pending_files.clear();
  return files;
}

bool MpzApplication::event(QEvent *event) {
  if (event->type() == QEvent::FileOpen) {
    auto *foe = static_cast<QFileOpenEvent *>(event);
    QString path = foe->file();
    if (path.isEmpty() && foe->url().isValid()) {
      path = foe->url().toLocalFile();
    }
    if (!path.isEmpty()) {
      if (receivers(SIGNAL(filesOpened(QStringList))) > 0) {
        emit filesOpened(QStringList{path});
      } else {
        pending_files << path;
      }
      return true;
    }
  }
  return QApplication::event(event);
}
