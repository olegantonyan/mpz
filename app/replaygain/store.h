#ifndef REPLAYGAIN_STORE_H
#define REPLAYGAIN_STORE_H

#include "replaygain/gain.h"

#include <sqlite3.h>

#include <QElapsedTimer>
#include <QFileInfo>
#include <QPair>
#include <QString>
#include <QStringList>

namespace ReplayGain {
  // A gain per (path, slice), kept in an on-disk B-tree. Nothing about it scales with the size of the library:
  // opening reads a page or two, a lookup is one descent, and memory is whatever the page cache is set to.
  class Store {
  public:
    using Key = QPair<QString, quint64>;

    explicit Store(const QString &dir);
    ~Store();

    Store(const Store &) = delete;
    Store &operator=(const Store &) = delete;

    bool isOpen() const { return db != nullptr; }
    QString filePath() const { return filepath; }

    Gain get(const QString &path, quint64 begin_ms) const;
    Gain get(const QString &path, const QFileInfo &info, quint64 begin_ms) const;
    bool put(const QString &path, quint64 begin_ms, const Gain &gain);

    qint64 count() const;

    // A scan wraps its writes in one transaction instead of paying a commit per track.
    void beginBatch();
    void endBatch();

    // Drops rows for files that are gone, but only under folders just walked, and never
    // for a folder that has itself disappeared — an unmounted drive must not wipe the db.
    int pruneMissing(const QStringList &folders);

  private:
    enum class OpenResult { Ok, Failed, TooNew };

    OpenResult openAt(const QString &path);
    void close();
    bool exec(const char *sql);
    void quarantine();
    void commitIfDue();
    Gain lookup(const QString &path, quint64 begin_ms, const QFileInfo *known) const;

    QString filepath;
    sqlite3 *db = nullptr;
    sqlite3_stmt *get_stmt = nullptr;
    sqlite3_stmt *put_stmt = nullptr;
    sqlite3_stmt *count_stmt = nullptr;

    bool in_batch = false;
    bool in_transaction = false;
    int uncommitted = 0;
    QElapsedTimer since_commit;
  };
}

#endif // REPLAYGAIN_STORE_H
