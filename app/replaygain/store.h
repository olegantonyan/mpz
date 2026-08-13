#ifndef REPLAYGAIN_STORE_H
#define REPLAYGAIN_STORE_H

#include "replaygain/gain.h"

#include <QFile>
#include <QHash>
#include <QString>

namespace ReplayGain {
  class Store {
  public:
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

    static QString key(const QString &path, quint64 begin_ms);

    Gain get(const QString &path, quint64 begin_ms) const;
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
    QHash<QString, Entry> entries;
    QFile out;
    int live_lines = 0;
    int dead_lines = 0;
    bool needs_rewrite = false;
  };
}

#endif // REPLAYGAIN_STORE_H
