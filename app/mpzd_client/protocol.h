#ifndef MPZD_CLIENT_PROTOCOL_H
#define MPZD_CLIENT_PROTOCOL_H

#include "track.h"
#include "playlist/playlist.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

namespace MpzdClient {
  constexpr int DEFAULT_PORT = 6601;
  constexpr int PROTOCOL_VERSION = 1;

  using Record = QHash<QString, QString>;

  struct Reply {
    bool ok = false;
    int code = 0;
    QString message;
    QList<QPair<QString, QString>> lines;

    QString value(const QString &key) const;
    QList<Record> records(const QString &first_key) const;
  };

  struct Item {
    enum Kind { Greeting, Event, Done };
    Kind kind;
    QString text;
    Reply reply;
  };

  class Parser {
  public:
    QList<Item> feed(const QByteArray &data);
    void reset();

  private:
    QByteArray buffer;
    Reply current;
    bool greeted = false;
  };

  struct Status {
    enum State { Stop, Play, Pause };
    State state = Stop;
    quint32 playlist = 0;
    quint32 track = 0;
    Record track_record;
    quint32 pos_ms = 0;
    QString format;
    QString icy;
    int volume = 100;
    QString order;
    bool follow = false;
    quint32 sel_playlist = 0;
    quint32 sel_track = 0;

    static Status parse(const Reply &reply);
  };

  struct PlaylistInfo {
    quint32 id = 0;
    QString name;
    quint32 count = 0;
    QString order;
    quint32 scanning = 0;
  };

  QByteArray encode(const QStringList &args);
  QString quote(const QString &arg);

  quint64 trackUid(quint32 playlist_id, quint32 track_id);
  quint32 playlistIdOf(quint64 uid);
  quint32 trackIdOf(quint64 uid);

  Track trackFromRecord(const Record &rec, quint32 playlist_id, const QString &playlist_name, const QUrl &server);
  QVector<Track> tracksFromReply(const Reply &reply, quint32 playlist_id, const QString &playlist_name, const QUrl &server);
  QList<PlaylistInfo> playlistsFromReply(const Reply &reply);

  Playlist::Playlist::PlaylistRandom playlistOrderFrom(const QString &order);
  QString playlistOrderName(Playlist::Playlist::PlaylistRandom order);
  QString globalOrderFrom(const QString &order);
  QString globalOrderName(const QString &mpz_order);
}

#endif // MPZD_CLIENT_PROTOCOL_H
