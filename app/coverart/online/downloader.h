#ifndef COVERART_ONLINE_DOWNLOADER_H
#define COVERART_ONLINE_DOWNLOADER_H

#include "coverart/online/albumquery.h"
#include "track.h"
#include "config/global.h"

#include <QObject>
#include <QSet>
#include <QString>

namespace CoverArt {
  namespace Online {
    // Owns every online cover lookup. Driven from the playback signals in
    // MainWindow rather than from Track::artCover(), so "only the current track
    // is searched" is a property of the wiring instead of a rule callers have to
    // remember. Consumers listen for coverAvailable and re-read artCover().
    class Downloader : public QObject {
      Q_OBJECT
    public:
      static Downloader &instance();

      // Must be called once at startup with the single app-wide config before
      // any request(); request() is a no-op until then.
      void init(Config::Global &global);

      // No-op unless the track needs a cover and online providers are enabled.
      // Safe to call repeatedly for the same track.
      void request(const Track &track);

      // Both a query and a signal, so listeners work whether they are wired up
      // before or after request() runs for a given track.
      bool isSearching(const QString &artist, const QString &album) const;

    signals:
      void searchStarted(const QString &artist, const QString &album);
      void coverAvailable(const QString &artist, const QString &album, const QString &path);
      void searchFinished(const QString &artist, const QString &album);

    private:
      explicit Downloader(QObject *parent = nullptr);

      void start(const AlbumQuery &query, const QStringList &providers);

      Config::Global *global_conf = nullptr;
      QSet<QString> in_flight;
    };
  }
}

#endif // COVERART_ONLINE_DOWNLOADER_H
