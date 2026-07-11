#include "coverartwidget.h"

#include "icons.h"

#include <QResizeEvent>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include <QPalette>

namespace CoverArt {
  Widget::Widget(QWidget *parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setWordWrap(true);
    setMinimumSize(80, 80);
    // muted, small placeholder text (not the cover pixmap)
    setForegroundRole(QPalette::PlaceholderText);
    QFont f = font();
    if (f.pointSizeF() > 0) {
      f.setPointSizeF(f.pointSizeF() * 0.85);
    }
    setFont(f);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &Widget::showContextMenu);
    clear();
  }

  void Widget::setTrack(const Track &track) {
    _track = track;
    const QString path = track.artCover();
    QPixmap cover(path);
    if (path.isEmpty() || cover.isNull()) {
      _cover_path.clear();
      source = QPixmap();
      setText(tr("No cover art"));
      return;
    }
    _cover_path = path;
    source = cover;
    render();
  }

  void Widget::clear() {
    _track = Track();
    _cover_path.clear();
    source = QPixmap();
    setText(tr("Nothing playing"));
  }

  void Widget::showContextMenu(const QPoint &pos) {
    if (!_track.isValid()) {
      return;
    }

    QMenu menu(this);

    QAction viewer(tr("Open in external viewer"), &menu);
    viewer.setIcon(Icons::get(Icons::Icon::FolderReveal));
    viewer.setEnabled(!_cover_path.isEmpty());
    connect(&viewer, &QAction::triggered, this, [this]() {
      if (!_cover_path.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(_cover_path));
      }
    });

    QAction info(tr("Track info"), &menu);
    info.setIcon(Icons::get(Icons::Icon::Info));
    connect(&info, &QAction::triggered, this, [this]() {
      emit trackInfoRequested(_track);
    });

    menu.addAction(&viewer);
    menu.addAction(&info);
    menu.exec(mapToGlobal(pos));
  }

  void Widget::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    if (!source.isNull()) {
      render();
    }
  }

  void Widget::render() {
    if (source.isNull()) {
      return;
    }
    setPixmap(source.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }
}
