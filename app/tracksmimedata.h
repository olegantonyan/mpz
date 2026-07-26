#ifndef TRACKSMIMEDATA_H
#define TRACKSMIMEDATA_H

#include "track.h"

#include <QByteArray>
#include <QMimeData>
#include <QString>
#include <QVector>

// Carries live Track objects, so it only works for drags that stay inside the process.
class TracksMimeData : public QMimeData {
public:
  static inline const QString format = QStringLiteral("application/x-mpz-tracks");

  TracksMimeData(const QVector<Track> &tracks, const QString &suggested_name, quint64 source_playlist_uid = 0) :
    tracks_list(tracks), name(suggested_name), source_uid(source_playlist_uid) {
    setData(format, QByteArray());
  }

  QVector<Track> tracks() const {
    return tracks_list;
  }

  QString suggestedName() const {
    return name;
  }

  quint64 sourcePlaylistUid() const {
    return source_uid;
  }

  static const TracksMimeData *from(const QMimeData *data) {
    return dynamic_cast<const TracksMimeData *>(data);
  }

private:
  QVector<Track> tracks_list;
  QString name;
  quint64 source_uid;
};

#endif // TRACKSMIMEDATA_H
