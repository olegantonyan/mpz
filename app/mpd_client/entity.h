#ifndef MPD_CLIENT_ENTITY_H
#define MPD_CLIENT_ENTITY_H

#include <QObject>
#include <QDebug>
#include <QDateTime>

#include "mpd/client.h"

namespace MpdClient {
  class Entity {
    Q_GADGET
  public:
    enum Type {
      ENTITY_UNKNOWN,
      ENTITY_DIR,
      ENTITY_SONG,
      ENTITY_PLAYLIST
    };
    Q_ENUM(Type)

    Entity() = default;
    explicit Entity(const struct mpd_entity *entity);
    explicit Entity(const struct mpd_playlist *playlist);
    explicit Entity(Type type, const QString &path, time_t modifiet_at);

    void updateFromMpdEntity(const struct mpd_entity *entity);
    void updateFromMpdPlaylist(const struct mpd_playlist *playlist);

    bool isValid() const;
    bool isDir() const;
    Type type() const;
    QString path() const;
    time_t modified_at() const;
    QDateTime modified_at_datetime() const;

  private:
    Type _type = ENTITY_UNKNOWN;
    QString _path = "";
    time_t _modified_at = 0;
  };

  QDebug operator<<(QDebug dbg, const Entity &e);
}

Q_DECLARE_METATYPE(MpdClient::Entity)

#endif
