#include "lyricswidget.h"

#include "config/global.h"
#include "lyrics/lrcparser.h"
#include "lyrics/providerchain.h"
#include "lyrics/trackquery.h"
#include "icons.h"

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QAction>

namespace Lyrics {
  Widget::Widget(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    source_label = new QLabel(this);
    text = new QPlainTextEdit(this);
    text->setReadOnly(true);
    text->setFrameShape(QFrame::NoFrame);
    text->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(text, &QWidget::customContextMenuRequested, this, &Widget::showContextMenu);

    layout->addWidget(source_label);
    layout->addWidget(text);

    clear();
  }

  void Widget::showContextMenu(const QPoint &pos) {
    QMenu *menu = text->createStandardContextMenu();
    if (_track.isValid()) {
      menu->addSeparator();
      QAction *info = menu->addAction(Icons::get(Icons::Icon::Info), tr("Track info"));
      connect(info, &QAction::triggered, this, [this]() {
        emit trackInfoRequested(_track);
      });
    }
    menu->exec(text->mapToGlobal(pos));
    delete menu;
  }

  void Widget::setTrack(const Track &track) {
    cancel_pending();
    _track = track;

    if (track.isStream() || (track.artist().isEmpty() && track.title().isEmpty())) {
      render_state(tr("No lyrics found."));
      return;
    }

    QString embedded = fetch_embedded_lyrics(track);
    if (!embedded.isEmpty()) {
      render_lyrics("embedded", embedded);
      return;
    }
    QString sidecar = fetch_sidecar_lyrics(track);
    if (!sidecar.isEmpty()) {
      render_lyrics("sidecar", sidecar);
      return;
    }

    Config::Global global;
    const auto online = ProviderChain::filterKnown(global.lyricsProviders());
    if (!online.isEmpty() && !track.artist().isEmpty() && !track.title().isEmpty()) {
      render_state(tr("Searching lyrics..."));
      chain = new ProviderChain(this);
      connect(chain, &ProviderChain::found, this, [this](const QString &provider, const QString &lyrics) {
        render_lyrics(ProviderChain::displayName(provider), lyrics);
        cancel_pending();
      });
      connect(chain, &ProviderChain::notFound, this, [this]() {
        render_state(tr("No lyrics found."));
        cancel_pending();
      });
      chain->fetch(online, TrackQuery{track.artist(), track.title(), track.album(),
                                      static_cast<int>(track.duration() / 1000)});
      return;
    }

    render_state(tr("No lyrics found."));
  }

  void Widget::clear() {
    cancel_pending();
    _track = Track();
    render_state(tr("Nothing playing"));
  }

  void Widget::cancel_pending() {
    if (chain) {
      disconnect(chain, nullptr, this, nullptr);
      chain->deleteLater();
      chain = nullptr;
    }
  }

  QString Widget::fetch_embedded_lyrics(const Track &track) const {
    if (track.isMpd() || track.isCue() || track.path().isEmpty() || !QFile::exists(track.path())) {
      return QString();
    }
    TagLib::FileRef f(track.path().toUtf8().constData());
    if (f.isNull() || !f.tag()) {
      return QString();
    }
    auto props = f.tag()->properties();
    auto it = props.find("LYRICS");
    if (it == props.end() || it->second.isEmpty()) {
      return QString();
    }
    return QString::fromUtf8(it->second.front().toCString(true));
  }

  QString Widget::fetch_sidecar_lyrics(const Track &track) const {
    if (track.isMpd() || track.path().isEmpty()) {
      return QString();
    }
    QFileInfo fi(track.path());
    for (const QString &ext : { QStringLiteral("lrc"), QStringLiteral("txt") }) {
      QString candidate = fi.path() + "/" + fi.completeBaseName() + "." + ext;
      if (!QFile::exists(candidate)) {
        continue;
      }
      QFile f(candidate);
      if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(f.readAll());
      }
    }
    return QString();
  }

  void Widget::render_lyrics(const QString &source, const QString &raw) {
    source_label->setText(QString("(%1)").arg(source));
    const QString body = LrcParser::looksLikeLrc(raw)
                           ? LrcParser::stripTimestamps(raw)
                           : raw.trimmed();
    text->setPlainText(body);
  }

  void Widget::render_state(const QString &message) {
    source_label->clear();
    text->clear();
    text->setPlaceholderText(message);
  }
}
