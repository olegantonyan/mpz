#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include "track.h"

#include <QString>
#include <QDir>
#include <QVector>
#include <QObject>
#include <QFuture>

namespace Playlist {
  class Playlist : public QObject {
    Q_OBJECT
  public:
    enum PlaylistRandom {
      None = 0,
      Random,
      Sequential
    };

    explicit Playlist();

    static QStringList supportedFileFormats();
    static QStringList supportedPlaylistFileFormats();

    QString name() const;
    QString rename(const QString &value);
    QVector<Track> tracks() const;

    bool load(const QDir &path);
    void loadAsync(const QDir &path);
    bool load(const QVector<Track> &tracks);
    bool concat(const QDir &path);
    void concatAsync(const QDir &path);

    quint64 uid() const;

    bool hasTrack(quint64 track_uid) const;
    int trackIndex(quint64 track_uid) const;
    Track trackBy(quint64 uid) const;

    void sort();

    void removeTrack(int position);

    enum PlaylistRandom random() const;
    void setRandom(enum PlaylistRandom arg);

  signals:
    void loadAsyncFinished(Playlist *pl);
    void concatAsyncFinished(Playlist *pl);

  private:
    QString playlist_name;
    QVector<Track> tracks_list;
    quint64 _uid;
    enum PlaylistRandom _random;
  };
}

#endif // PLAYLISTITEM_H
