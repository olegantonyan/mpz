#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QString>
#include <QDir>

class Playlist {
public:
  explicit Playlist(const QDir &path);

  QDir path() const;

  QString name() const;
  void rename(const QString &value);

private:
  QDir directory_path;
  QString playlist_name;
};

#endif // PLAYLISTITEM_H
