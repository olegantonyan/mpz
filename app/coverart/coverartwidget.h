#ifndef COVERART_WIDGET_H
#define COVERART_WIDGET_H

#include "track.h"

#include <QLabel>
#include <QPixmap>
#include <QString>

namespace CoverArt {
  class Widget : public QLabel {
    Q_OBJECT
  public:
    explicit Widget(QWidget *parent = nullptr);

  public slots:
    void setTrack(const Track &track);
    void clear();

  signals:
    void trackInfoRequested(const Track &track);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private slots:
    void showContextMenu(const QPoint &pos);
    void onSearchStarted(const QString &artist, const QString &album);
    void onCoverDownloaded(const QString &artist, const QString &album, const QString &path);
    void onSearchFinished(const QString &artist, const QString &album);

  private:
    void render();
    void render_cover();
    bool isCurrent(const QString &artist, const QString &album) const;

    QPixmap source;
    Track _track;
    QString _cover_path;
  };
}

#endif // COVERART_WIDGET_H
