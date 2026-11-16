#include "protocol.h"

namespace MpzdClient {
  QString Reply::value(const QString &key) const {
    for (const auto &line : lines) {
      if (line.first == key) {
        return line.second;
      }
    }
    return QString();
  }

  QList<Record> Reply::records(const QString &first_key) const {
    QList<Record> result;
    for (const auto &line : lines) {
      if (line.first == first_key) {
        result.append(Record());
      }
      if (!result.isEmpty()) {
        result.last().insert(line.first, line.second);
      }
    }
    return result;
  }

  void Parser::reset() {
    buffer.clear();
    current = Reply();
    greeted = false;
  }

  QList<Item> Parser::feed(const QByteArray &data) {
    QList<Item> items;
    buffer.append(data);
    int start = 0;
    while (true) {
      const int nl = buffer.indexOf('\n', start);
      if (nl < 0) {
        break;
      }
      QByteArray raw = buffer.mid(start, nl - start);
      start = nl + 1;
      if (raw.endsWith('\r')) {
        raw.chop(1);
      }
      const QString line = QString::fromUtf8(raw);
      if (!greeted) {
        greeted = true;
        items.append({Item::Greeting, line, Reply()});
        continue;
      }
      if (line.startsWith("! ") && current.lines.isEmpty()) {
        items.append({Item::Event, line.mid(2), Reply()});
        continue;
      }
      if (line == "ok") {
        current.ok = true;
        items.append({Item::Done, QString(), current});
        current = Reply();
        continue;
      }
      if (line.startsWith("err ")) {
        const int sp = line.indexOf(' ', 4);
        current.ok = false;
        current.code = line.mid(4, sp < 0 ? -1 : sp - 4).toInt();
        current.message = sp < 0 ? QString() : line.mid(sp + 1);
        items.append({Item::Done, QString(), current});
        current = Reply();
        continue;
      }
      const int colon = line.indexOf(": ");
      if (colon > 0) {
        current.lines.append({line.left(colon), line.mid(colon + 2)});
      } else if (line.endsWith(':')) {
        current.lines.append({line.chopped(1), QString()});
      }
    }
    buffer.remove(0, start);
    return items;
  }

  Status Status::parse(const Reply &reply) {
    Status st;
    const QString state = reply.value("state");
    st.state = state == "play" ? Play : (state == "pause" ? Pause : Stop);
    bool in_track = false;
    for (const auto &line : reply.lines) {
      const QString &k = line.first;
      const QString &v = line.second;
      if (k == "playlist") {
        st.playlist = v.toUInt();
      } else if (k == "track") {
        st.track = v.toUInt();
        in_track = true;
      } else if (k == "pos_ms") {
        st.pos_ms = v.toUInt();
        in_track = false;
      } else if (k == "format") {
        st.format = v;
      } else if (k == "icy") {
        st.icy = v;
      } else if (k == "volume") {
        st.volume = v.toInt();
        in_track = false;
      } else if (k == "output") {
        st.output = v;
      } else if (k == "order") {
        st.order = v;
      } else if (k == "follow") {
        st.follow = v == "1";
      } else if (k == "sel_playlist") {
        st.sel_playlist = v.toUInt();
      } else if (k == "sel_track") {
        st.sel_track = v.toUInt();
      }
      if (in_track) {
        st.track_record.insert(k, v);
      }
    }
    return st;
  }

  QString quote(const QString &arg) {
    bool plain = !arg.isEmpty();
    for (const QChar c : arg) {
      if (c.isSpace() || c == '"' || c == '\\') {
        plain = false;
        break;
      }
    }
    if (plain) {
      return arg;
    }
    QString out = "\"";
    for (const QChar c : arg) {
      if (c == '"' || c == '\\') {
        out += '\\';
      }
      out += (c == '\n' || c == '\r') ? QChar(' ') : c;
    }
    out += '"';
    return out;
  }

  QByteArray encode(const QStringList &args) {
    QStringList quoted;
    for (const auto &a : args) {
      quoted << quote(a);
    }
    return quoted.join(' ').toUtf8() + '\n';
  }

  quint64 trackUid(quint32 playlist_id, quint32 track_id) {
    return (static_cast<quint64>(playlist_id) << 32) | track_id;
  }

  quint32 playlistIdOf(quint64 uid) {
    return static_cast<quint32>(uid >> 32);
  }

  quint32 trackIdOf(quint64 uid) {
    return static_cast<quint32>(uid & 0xffffffffu);
  }

  Track trackFromRecord(const Record &rec, quint32 playlist_id, const QString &playlist_name, const QUrl &server) {
    const QString file = rec.value("file");
    Track track;
    if (rec.value("stream") == "1") {
      track = Track(QUrl(file), file);
    } else {
      track = Track(file,
                    rec.value("begin_ms").toULongLong(),
                    rec.value("artist"),
                    rec.value("album"),
                    rec.value("title"),
                    static_cast<quint16>(rec.value("number").toUInt()),
                    static_cast<quint16>(rec.value("year").toUInt()),
                    rec.value("dur_ms").toULongLong(),
                    0, 0, 0);
      track.setCue(rec.contains("begin_ms"));
      track.setAlbumArtist(rec.value("album_artist"));
      if (rec.contains("disc")) {
        track.setDiscNumber(rec.value("disc"));
      }
    }
    track.setUid(trackUid(playlist_id, rec.value("track").toUInt()));
    track.setPlaylistName(playlist_name);
    track.setRemote(server);
    return track;
  }

  QVector<Track> tracksFromReply(const Reply &reply, quint32 playlist_id, const QString &playlist_name, const QUrl &server) {
    QVector<Track> result;
    const auto recs = reply.records("track");
    result.reserve(recs.size());
    for (const auto &rec : recs) {
      result.append(trackFromRecord(rec, playlist_id, playlist_name, server));
    }
    return result;
  }

  QList<PlaylistInfo> playlistsFromReply(const Reply &reply) {
    QList<PlaylistInfo> result;
    for (const auto &rec : reply.records("playlist")) {
      PlaylistInfo pl;
      pl.id = rec.value("playlist").toUInt();
      pl.name = rec.value("name");
      pl.count = rec.value("count").toUInt();
      pl.order = rec.value("order");
      pl.scanning = rec.value("scanning").toUInt();
      result.append(pl);
    }
    return result;
  }

  Playlist::Playlist::PlaylistRandom playlistOrderFrom(const QString &order) {
    if (order == "random") {
      return Playlist::Playlist::Random;
    }
    if (order == "seq") {
      return Playlist::Playlist::Sequential;
    }
    if (order == "noloop") {
      return Playlist::Playlist::SequentialNoLoop;
    }
    return Playlist::Playlist::None;
  }

  QString playlistOrderName(Playlist::Playlist::PlaylistRandom order) {
    switch (order) {
    case Playlist::Playlist::Random:
      return "random";
    case Playlist::Playlist::Sequential:
      return "seq";
    case Playlist::Playlist::SequentialNoLoop:
      return "noloop";
    case Playlist::Playlist::None:
    default:
      return "inherit";
    }
  }

  QString globalOrderFrom(const QString &order) {
    if (order == "random") {
      return "random";
    }
    if (order == "noloop") {
      return "sequential (no loop)";
    }
    return "sequential";
  }

  QString globalOrderName(const QString &mpz_order) {
    if (mpz_order == "random") {
      return "random";
    }
    if (mpz_order == "sequential (no loop)") {
      return "noloop";
    }
    return "seq";
  }
}
