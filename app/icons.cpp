#include "icons.h"

#include <QApplication>
#include <QHash>
#include <QIconEngine>
#include <QImage>
#include <QPaintDevice>
#include <QPainter>
#include <QPalette>
#include <QRectF>
#include <QString>
#include <QSvgRenderer>

namespace {
  QString resourcePath(Icons::Icon icon) {
    switch (icon) {
      case Icons::Icon::Play:         return QStringLiteral(":/icons/ui/play.svg");
      case Icons::Icon::Pause:        return QStringLiteral(":/icons/ui/pause.svg");
      case Icons::Icon::Stop:         return QStringLiteral(":/icons/ui/stop.svg");
      case Icons::Icon::Next:         return QStringLiteral(":/icons/ui/next.svg");
      case Icons::Icon::Prev:         return QStringLiteral(":/icons/ui/prev.svg");
      case Icons::Icon::Volume:       return QStringLiteral(":/icons/ui/volume.svg");
      case Icons::Icon::VolumeMuted:  return QStringLiteral(":/icons/ui/volume-muted.svg");
      case Icons::Icon::Headphones:   return QStringLiteral(":/icons/ui/headphones.svg");
      case Icons::Icon::Settings:     return QStringLiteral(":/icons/ui/settings.svg");
      case Icons::Icon::Menu:         return QStringLiteral(":/icons/ui/menu.svg");
      case Icons::Icon::Sort:         return QStringLiteral(":/icons/ui/sort.svg");
      case Icons::Icon::Trash:        return QStringLiteral(":/icons/ui/trash.svg");
      case Icons::Icon::Folder:       return QStringLiteral(":/icons/ui/folder.svg");
      case Icons::Icon::FolderReveal: return QStringLiteral(":/icons/ui/folder-reveal.svg");
      case Icons::Icon::Reload:       return QStringLiteral(":/icons/ui/reload.svg");
      case Icons::Icon::Save:         return QStringLiteral(":/icons/ui/save.svg");
      case Icons::Icon::Info:         return QStringLiteral(":/icons/ui/info.svg");
      case Icons::Icon::Details:      return QStringLiteral(":/icons/ui/details.svg");
      case Icons::Icon::Help:         return QStringLiteral(":/icons/ui/help.svg");
      case Icons::Icon::Cancel:       return QStringLiteral(":/icons/ui/cancel.svg");
      case Icons::Icon::Copy:         return QStringLiteral(":/icons/ui/copy.svg");
      case Icons::Icon::Edit:         return QStringLiteral(":/icons/ui/edit.svg");
      case Icons::Icon::OpenFile:     return QStringLiteral(":/icons/ui/open-file.svg");
      case Icons::Icon::NewPlaylist:  return QStringLiteral(":/icons/ui/new-playlist.svg");
      case Icons::Icon::AddToPlaylist:return QStringLiteral(":/icons/ui/add-to-playlist.svg");
      case Icons::Icon::Spinner:      return QStringLiteral(":/icons/ui/spinner.svg");
    }
    return QString();
  }

  QColor colorForMode(QIcon::Mode mode) {
    const QPalette pal = qApp->palette();
    switch (mode) {
      case QIcon::Disabled: return pal.color(QPalette::Disabled, QPalette::WindowText);
      case QIcon::Selected: return pal.color(QPalette::Active, QPalette::HighlightedText);
      default:              return pal.color(QPalette::Active, QPalette::WindowText);
    }
  }

  class TintedSvgIconEngine : public QIconEngine {
  public:
    explicit TintedSvgIconEngine(const QString &resource) : resource(resource), renderer(resource) {}

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override {
      Q_UNUSED(state)
      const qreal dpr = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
      painter->drawPixmap(rect, renderTinted(rect.size(), dpr, mode));
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
      Q_UNUSED(state)
      return renderTinted(size, 1.0, mode);
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override {
      Q_UNUSED(state)
      return renderTinted(size, scale, mode);
    }
#else
    void virtual_hook(int id, void *data) override {
      if (id == QIconEngine::ScaledPixmapHook) {
        auto *arg = reinterpret_cast<QIconEngine::ScaledPixmapArgument *>(data);
        arg->pixmap = renderTinted(arg->size, arg->scale, arg->mode);
      } else {
        QIconEngine::virtual_hook(id, data);
      }
    }
#endif

    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
      Q_UNUSED(mode)
      Q_UNUSED(state)
      return size;
    }

    QIconEngine *clone() const override {
      return new TintedSvgIconEngine(resource);
    }

    QString key() const override {
      return QStringLiteral("TintedSvgIconEngine");
    }

  private:
    QPixmap renderTinted(const QSize &logical, qreal dpr, QIcon::Mode mode) {
      if (logical.isEmpty() || !renderer.isValid()) {
        return QPixmap();
      }
      const QColor color = colorForMode(mode);
      const QString ck = QStringLiteral("%1x%2@%3:%4").arg(logical.width()).arg(logical.height()).arg(dpr).arg(color.rgba());
      const auto it = cache.constFind(ck);
      if (it != cache.constEnd()) {
        return it.value();
      }
      const int w = qRound(logical.width() * dpr);
      const int h = qRound(logical.height() * dpr);
      QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
      img.fill(Qt::transparent);
      {
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&p, QRectF(0, 0, w, h));
      }
      {
        // keep the rendered glyph's alpha, replace its (black) color with the theme color
        QPainter p(&img);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(img.rect(), color);
      }
      QPixmap pm = QPixmap::fromImage(img);
      pm.setDevicePixelRatio(dpr);
      cache.insert(ck, pm);
      return pm;
    }

    QString resource;
    QSvgRenderer renderer;
    QHash<QString, QPixmap> cache;
  };
}

namespace Icons {
  QIcon get(Icon icon) {
    static QHash<int, QIcon> icons;
    const int key = static_cast<int>(icon);
    const auto it = icons.constFind(key);
    if (it != icons.constEnd()) {
      return it.value();
    }
    QIcon qicon(new TintedSvgIconEngine(resourcePath(icon)));
    icons.insert(key, qicon);
    return qicon;
  }

  QPixmap pixmap(Icon icon, const QSize &size, QIcon::Mode mode) {
    return get(icon).pixmap(size, mode);
  }
}
