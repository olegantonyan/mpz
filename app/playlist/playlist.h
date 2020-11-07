#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include "track.h"
#include "playlist/sorter.h"

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

    QString name() const;
    QString rename(const QString &value);
    QVector<Track> tracks() const;

    void load(const QDir &path);
    void load(const QVector<Track> &tracks);
    void load(const QList<QDir> &dirs);
    void loadAsync(const QList<QDir> &dirs);
    void concat(const QDir &path);
    void concat(const QList<QDir> &dirs);
    void concatAsync(const QList<QDir> &dirs);

    quint64 uid() const;

    bool hasTrack(quint64 track_uid) const;
    int trackIndex(quint64 track_uid) const;
    Track trackBy(quint64 uid) const;

    void removeTrack(int position);

    enum PlaylistRandom random() const;
    void setRandom(enum PlaylistRandom arg);

    void sortBy(const QString &criteria);

    QByteArray toM3U() const;

    void reload();

  signals:
    void loadAsyncFinished(Playlist *pl);
    void concatAsyncFinished(Playlist *pl);

  private:
    QVector<Track> sort(QVector<Track> list, const Sorter &sorter = Sorter());
    QString nameBy(const QDir &path);

    QString playlist_name;
    QVector<Track> tracks_list;
    quint64 _uid;
    enum PlaylistRandom _random;
  };
}

#endif // PLAYLISTITEM_H
