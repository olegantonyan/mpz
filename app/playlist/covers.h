#ifndef PLAYLIST_COVERS_H
#define PLAYLIST_COVERS_H

#include <QObject>
#include <QString>
#include <QHash>

namespace Playlist {
  class Covers : public QObject {
    Q_OBJECT
  public:
    static Covers &instance();

    QString get(const QString& filepath);

  private:
    explicit Covers(QObject *parent = nullptr);

    QString keyByFilepath(const QString& filepath) const;
    QString findCoverLocally(const QString &dir);

    QHash <QString, QString> cache;
  };
}

#endif // PLAYLIST_COVERS_H
