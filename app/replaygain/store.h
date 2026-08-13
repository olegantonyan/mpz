#ifndef REPLAYGAIN_STORE_H
#define REPLAYGAIN_STORE_H

#include "replaygain/gain.h"

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QPair>
#include <QString>

namespace ReplayGain {
  class Store {
  public:
    using Key = QPair<QString, quint64>;

    struct Entry {
      QString path;
      quint64 begin_ms = 0;
      Gain gain;
      qint64 mtime = 0;
      qint64 size = 0;
    };

    explicit Store(const QString &dir);
    ~Store();

    Store(const Store &) = delete;
    Store &operator=(const Store &) = delete;

    Gain get(const QString &path, quint64 begin_ms) const;
    Gain get(const QString &path, const QFileInfo &info, quint64 begin_ms) const;
    bool put(const QString &path, quint64 begin_ms, const Gain &gain);

    int count() const { return entries.size(); }
    QString filePath() const { return filepath; }

    bool reload();
    bool compact();
    void compactIfNeeded();

  private:
    bool appendLine(const QString &line);
    void closeOutput();

    QString filepath;
    QHash<Key, Entry> entries;
    QFile out;
    int live_lines = 0;
    int dead_lines = 0;
    bool needs_rewrite = false;
  };
}

#endif // REPLAYGAIN_STORE_H
