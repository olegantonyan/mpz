#include "coverartwidget.h"

#include <QResizeEvent>

namespace CoverArt {
  Widget::Widget(QWidget *parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setWordWrap(true);
    setMinimumSize(80, 80);
    clear();
  }

  void Widget::setTrack(const Track &track) {
    const QString path = track.artCover();
    QPixmap cover(path);
    if (path.isEmpty() || cover.isNull()) {
      clear();
      return;
    }
    source = cover;
    render();
  }

  void Widget::clear() {
    source = QPixmap();
    setText(tr("No cover art"));
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
