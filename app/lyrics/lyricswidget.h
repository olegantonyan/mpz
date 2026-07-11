#ifndef LYRICS_WIDGET_H
#define LYRICS_WIDGET_H

#include "track.h"

#include <QWidget>

class QLabel;
class QPlainTextEdit;

namespace Lyrics {
  class ProviderChain;

  class Widget : public QWidget {
    Q_OBJECT
  public:
    explicit Widget(QWidget *parent = nullptr);

  public slots:
    void setTrack(const Track &track);
    void clear();

  private:
    QLabel *source_label;
    QPlainTextEdit *text;
    ProviderChain *chain = nullptr;

    QString fetch_embedded_lyrics(const Track &track) const;
    QString fetch_sidecar_lyrics(const Track &track) const;
    void render_lyrics(const QString &source, const QString &raw);
    void render_state(const QString &message);
    void cancel_pending();
  };
}

#endif // LYRICS_WIDGET_H
