#include "entity.h"

#include <QMetaEnum>

namespace MpdClient {
  Entity::Entity(const mpd_entity *entity) {
    updateFromMpdEntity(entity);
  }

  Entity::Entity(Type type, const QString &path, time_t modifiet_at) : _type(type), _path(path), _modified_at(modifiet_at) {
  }

  void Entity::updateFromMpdEntity(const mpd_entity *entity) {
    if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY) {
      const struct mpd_directory* dir = mpd_entity_get_directory(entity);
      _type = Type::ENTITY_DIR;
      _path = QString::fromUtf8(mpd_directory_get_path(dir));
      _modified_at = mpd_directory_get_last_modified(dir);
    } else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
      const struct mpd_song* song = mpd_entity_get_song(entity);
      _type = Type::ENTITY_SONG;
      _path = QString::fromUtf8(mpd_song_get_uri(song));
      _modified_at = mpd_song_get_last_modified(song);
    }
  }

  bool Entity::isValid() const {
    return type() != ENTITY_UNKNOWN && !path().isEmpty();
  }

  Entity::Type Entity::type() const {
    return _type;
  }

  QString Entity::path() const {
    return _path;
  }

  time_t Entity::modified_at() const {
    return _modified_at;
  }

  QDebug operator<<(QDebug dbg, const Entity &e)
  {
      QDebugStateSaver saver(dbg);

      const QMetaObject &mo = Entity::staticMetaObject;
      int idx = mo.indexOfEnumerator("Type");
      QMetaEnum me = mo.enumerator(idx);
      const char *typeName = me.valueToKey(e.type());
      if (!typeName)
          typeName = "UNKNOWN";

      dbg.nospace() << "MpdClient::Entity("
                    << "type=" << typeName
                    << ", path=" << e.path()
                    << ", modified_at=" << e.modified_at()
                    << ")";

      return dbg;
  }
}
