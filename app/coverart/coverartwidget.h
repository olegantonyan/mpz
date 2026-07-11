#ifndef COVERART_WIDGET_H
#define COVERART_WIDGET_H

#include "track.h"

#include <QLabel>
#include <QPixmap>

namespace CoverArt {
  class Widget : public QLabel {
    Q_OBJECT
  public:
    explicit Widget(QWidget *parent = nullptr);

  public slots:
    void setTrack(const Track &track);
    void clear();

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    void render();

    QPixmap source;
  };
}

#endif // COVERART_WIDGET_H
