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
    bool load(const QVector<Track> &tracks);
    bool load(const QList<QDir> &dirs);
    void loadAsync(const QDir &path);
    void loadAsync(const QList<QDir> &dirs);
    bool concat(const QDir &path);
    bool concat(const QList<QDir> &dirs);
    void concatAsync(const QDir &path);
    void concatAsync(const QList<QDir> &dirs);

    quint64 uid() const;

    bool hasTrack(quint64 track_uid) const;
    int trackIndex(quint64 track_uid) const;
    Track trackBy(quint64 uid) const;

    void removeTrack(int position);

    enum PlaylistRandom random() const;
    void setRandom(enum PlaylistRandom arg);

  signals:
    void loadAsyncFinished(Playlist *pl);
    void concatAsyncFinished(Playlist *pl);

  private:
    QVector<Track> sort(QVector<Track> list);
    QString nameBy(const QDir &path);
    bool sortComparasion(const Track& t1, const Track& t2) const;

    QString playlist_name;
    QVector<Track> tracks_list;
    quint64 _uid;
    enum PlaylistRandom _random;
  };
}

#endif // PLAYLISTITEM_H
