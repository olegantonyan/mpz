#ifndef LYRICS_WIDGET_H
#define LYRICS_WIDGET_H

#include "track.h"
#include "config/global.h"
#include "lyrics/providerchain.h"

#include <QWidget>

class QLabel;
class QPlainTextEdit;

namespace Lyrics {
  class Widget : public QWidget {
    Q_OBJECT
  public:
    explicit Widget(Config::Global &global, QWidget *parent = nullptr);

  public slots:
    void setTrack(const Track &track);
    void clear();

  signals:
    void trackInfoRequested(const Track &track);

  private slots:
    void showContextMenu(const QPoint &pos);

  private:
    Config::Global &global_conf;
    QLabel *source_label;
    QPlainTextEdit *text;
    ProviderChain *chain = nullptr;
    Track _track;

    QString fetch_embedded_lyrics(const Track &track) const;
    QString fetch_sidecar_lyrics(const Track &track) const;
    void render_lyrics(const QString &source, const QString &raw);
    void render_state(const QString &message);
    void cancel_pending();
  };
}

#endif // LYRICS_WIDGET_H
