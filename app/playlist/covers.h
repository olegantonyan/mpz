#ifndef PLAYLIST_COVERS_H
#define PLAYLIST_COVERS_H

#include <QObject>
#include <QString>

namespace Playlist {
  class Covers : public QObject {
    Q_OBJECT
  public:
    static Covers &instance();

    QString get(const QString& filepath);

  private:
    explicit Covers(QObject *parent = nullptr);
  };
}

#endif // PLAYLIST_COVERS_H
