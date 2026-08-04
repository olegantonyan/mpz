#ifndef PLAYLISTFLOATINGHEADER_H
#define PLAYLISTFLOATINGHEADER_H

#include <QWidget>
#include <QEvent>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QStringList>
#include <QTableView>
#include <QTimer>
#include <QVector>

namespace PlaylistUi {
  class FloatingHeader : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

  public:
    explicit FloatingHeader(QTableView *v);

    void setColumns(const QStringList &labels, const QVector<Qt::Alignment> &aligns);
    void setActive(bool on);

    qreal opacity() const;
    void setOpacity(qreal v);
    int preferredHeight() const;

    static bool inRevealZone(int y, int header_height, bool visible);

    void onViewportEvent(QEvent *event);
    void onViewEvent(QEvent *event);

  public slots:
    void syncGeometry();
    void invalidateCache();
    void reveal();
    void scheduleHide();
    void hideNow();
    void setSuppressed(bool on);

  protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

  private:
    void renderCache();
    void animateTo(qreal target, int duration_ms);
    void trackPointer(int y);

    QTableView *view;
    QStringList labels;
    QVector<Qt::Alignment> aligns;
    QPixmap cache;
    QPropertyAnimation *animation;
    QTimer hide_timer;
    qreal opacity_value = 0.0;
    bool suppressed = false;
    bool active = true;
  };
}

#endif // PLAYLISTFLOATINGHEADER_H
