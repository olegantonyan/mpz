#include "replaygain/store.h"

#include <QDateTime>
#include <QDir>
#include <QFile>

namespace ReplayGain {
  namespace {
    const int kSchemaVersion = 1;
    const int kCommitEveryRows = 2000;
    const int kCommitEveryMs = 2000;

    const char *kSchema = R"(
      CREATE TABLE IF NOT EXISTS gain (
        path       TEXT    NOT NULL,
        begin_ms   INTEGER NOT NULL,
        track_db   REAL,
        track_peak REAL,
        album_db   REAL,
        album_peak REAL,
        mtime      INTEGER NOT NULL,
        size       INTEGER NOT NULL,
        PRIMARY KEY (path, begin_ms)
      ) WITHOUT ROWID;

      CREATE TABLE IF NOT EXISTS meta (
        key TEXT PRIMARY KEY, value TEXT NOT NULL
      ) WITHOUT ROWID;

      INSERT OR IGNORE INTO meta(key, value) VALUES ('schema_version', '1');
      INSERT OR IGNORE INTO meta(key, value) VALUES ('row_count', '0');

      CREATE TRIGGER IF NOT EXISTS gain_inserted AFTER INSERT ON gain BEGIN
        UPDATE meta SET value = CAST(value AS INTEGER) + 1 WHERE key = 'row_count';
      END;
      CREATE TRIGGER IF NOT EXISTS gain_deleted AFTER DELETE ON gain BEGIN
        UPDATE meta SET value = CAST(value AS INTEGER) - 1 WHERE key = 'row_count';
      END;
    )";

    // upsert rather than INSERT OR REPLACE: REPLACE would skip the delete trigger and
    // leave row_count counting every rewrite as a new row
    const char *kPutSql = R"(
      INSERT INTO gain (path, begin_ms, track_db, track_peak, album_db, album_peak, mtime, size)
      VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)
      ON CONFLICT(path, begin_ms) DO UPDATE SET
        track_db = ?3, track_peak = ?4, album_db = ?5, album_peak = ?6, mtime = ?7, size = ?8
    )";

    const char *kGetSql =
        "SELECT track_db, track_peak, album_db, album_peak, mtime, size "
        "FROM gain WHERE path = ?1 AND begin_ms = ?2";

    const char *kCountSql = "SELECT CAST(value AS INTEGER) FROM meta WHERE key = 'row_count'";

    void bindText(sqlite3_stmt *s, int i, const QString &v) {
      const QByteArray utf8 = v.toUtf8();
      sqlite3_bind_text(s, i, utf8.constData(), utf8.size(), SQLITE_TRANSIENT);
    }

    void bindOptional(sqlite3_stmt *s, int i, double v, bool present) {
      if (present) {
        sqlite3_bind_double(s, i, v);
      } else {
        sqlite3_bind_null(s, i);
      }
    }
  }

  Store::Store(const QString &dir) {
    QDir().mkpath(dir);
    filepath = QString("%1/replaygain.sqlite").arg(dir);
    // a db written by a newer mpz is left exactly as it is; only an unreadable one
    // is moved aside and replaced
    if (openAt(filepath) == OpenResult::Failed) {
      quarantine();
      openAt(filepath);
    }
  }

  Store::~Store() {
    endBatch();
    close();
  }

  Store::OpenResult Store::openAt(const QString &path) {
    if (sqlite3_open(path.toUtf8().constData(), &db) != SQLITE_OK) {
      close();
      return OpenResult::Failed;
    }

    // page_size only takes effect while the database is still empty
    exec("PRAGMA page_size = 8192");
    exec("PRAGMA journal_mode = WAL");
    exec("PRAGMA synchronous = NORMAL");
    exec("PRAGMA cache_size = -8000");
    // no mmap: the 8 MB page cache is then the whole of our footprint, and the OS page
    // cache still absorbs the file without it showing up as ours
    exec("PRAGMA mmap_size = 0");
    exec("PRAGMA temp_store = MEMORY");

    if (!exec(kSchema)) {
      close();
      return OpenResult::Failed;
    }

    sqlite3_stmt *version = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT CAST(value AS INTEGER) FROM meta WHERE key = 'schema_version'",
                           -1, &version, nullptr) == SQLITE_OK) {
      const bool newer = sqlite3_step(version) == SQLITE_ROW &&
                         sqlite3_column_int(version, 0) > kSchemaVersion;
      sqlite3_finalize(version);
      if (newer) {
        close();
        return OpenResult::TooNew;
      }
    }

    if (sqlite3_prepare_v2(db, kGetSql, -1, &get_stmt, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, kPutSql, -1, &put_stmt, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, kCountSql, -1, &count_stmt, nullptr) != SQLITE_OK) {
      close();
      return OpenResult::Failed;
    }
    return OpenResult::Ok;
  }

  void Store::close() {
    sqlite3_finalize(get_stmt);
    sqlite3_finalize(put_stmt);
    sqlite3_finalize(count_stmt);
    get_stmt = put_stmt = count_stmt = nullptr;
    sqlite3_close(db);
    db = nullptr;
    in_transaction = false;
    uncommitted = 0;
  }

  bool Store::exec(const char *sql) {
    return db != nullptr && sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
  }

  // A file we cannot open is renamed, never deleted, so a damaged db can still be looked at.
  void Store::quarantine() {
    close();
    if (!QFile::exists(filepath)) {
      return;
    }
    const QString aside = QString("%1.corrupt-%2")
                              .arg(filepath, QString::number(QDateTime::currentSecsSinceEpoch()));
    QFile::rename(filepath, aside);
    QFile::rename(filepath + "-wal", aside + "-wal");
    QFile::rename(filepath + "-shm", aside + "-shm");
  }

  Gain Store::get(const QString &path, quint64 begin_ms) const {
    return lookup(path, begin_ms, nullptr);
  }

  Gain Store::get(const QString &path, const QFileInfo &info, quint64 begin_ms) const {
    return lookup(path, begin_ms, &info);
  }

  // The row is fetched before the file is stat'ed, so an unknown track costs no syscall.
  Gain Store::lookup(const QString &path, quint64 begin_ms, const QFileInfo *known) const {
    if (get_stmt == nullptr) {
      return Gain();
    }

    sqlite3_reset(get_stmt);
    bindText(get_stmt, 1, path);
    sqlite3_bind_int64(get_stmt, 2, sqlite3_int64(begin_ms));
    if (sqlite3_step(get_stmt) != SQLITE_ROW) {
      return Gain();
    }

    Gain gain;
    gain.has_track = sqlite3_column_type(get_stmt, 0) != SQLITE_NULL;
    gain.track_db = sqlite3_column_double(get_stmt, 0);
    gain.track_peak = sqlite3_column_double(get_stmt, 1);
    gain.has_album = sqlite3_column_type(get_stmt, 2) != SQLITE_NULL;
    gain.album_db = sqlite3_column_double(get_stmt, 2);
    gain.album_peak = sqlite3_column_double(get_stmt, 3);
    const qint64 mtime = sqlite3_column_int64(get_stmt, 4);
    const qint64 size = sqlite3_column_int64(get_stmt, 5);
    // a select left mid-row holds a read transaction open, which stops WAL checkpointing
    sqlite3_reset(get_stmt);

    const QFileInfo info = known != nullptr ? *known : QFileInfo(path);
    const bool current = info.exists() && mtime == info.lastModified().toSecsSinceEpoch() &&
                         size == info.size();
    return current ? gain : Gain();
  }

  bool Store::put(const QString &path, quint64 begin_ms, const Gain &gain) {
    if (put_stmt == nullptr || path.isEmpty() || !gain.isValid()) {
      return false;
    }
    const QFileInfo info(path);
    if (!info.exists()) {
      return false;
    }

    if (!in_transaction && exec("BEGIN")) {
      in_transaction = true;
      since_commit.start();
    }

    sqlite3_reset(put_stmt);
    bindText(put_stmt, 1, path);
    sqlite3_bind_int64(put_stmt, 2, sqlite3_int64(begin_ms));
    bindOptional(put_stmt, 3, gain.track_db, gain.has_track);
    bindOptional(put_stmt, 4, gain.track_peak, gain.has_track);
    bindOptional(put_stmt, 5, gain.album_db, gain.has_album);
    bindOptional(put_stmt, 6, gain.album_peak, gain.has_album);
    sqlite3_bind_int64(put_stmt, 7, info.lastModified().toSecsSinceEpoch());
    sqlite3_bind_int64(put_stmt, 8, info.size());

    if (sqlite3_step(put_stmt) != SQLITE_DONE) {
      return false;
    }
    uncommitted++;
    commitIfDue();
    return true;
  }

  void Store::commitIfDue() {
    if (!in_transaction) {
      return;
    }
    const bool due = uncommitted >= kCommitEveryRows ||
                     (since_commit.isValid() && since_commit.elapsed() >= kCommitEveryMs);
    if (in_batch && !due) {
      return;
    }
    exec("COMMIT");
    in_transaction = false;
    uncommitted = 0;
  }

  qint64 Store::count() const {
    if (count_stmt == nullptr) {
      return 0;
    }
    sqlite3_reset(count_stmt);
    const qint64 n = sqlite3_step(count_stmt) == SQLITE_ROW ? sqlite3_column_int64(count_stmt, 0) : 0;
    sqlite3_reset(count_stmt);
    return n;
  }

  void Store::beginBatch() {
    in_batch = true;
  }

  void Store::endBatch() {
    in_batch = false;
    commitIfDue();
  }

  int Store::pruneMissing(const QStringList &folders) {
    if (db == nullptr || folders.isEmpty()) {
      return 0;
    }

    // A range over the primary key, not LIKE: an ESCAPE clause turns off the index
    // optimisation and would make this a full scan of the whole library.
    sqlite3_stmt *list = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT path, begin_ms FROM gain WHERE path >= ?1 AND path < ?2",
                           -1, &list, nullptr) != SQLITE_OK) {
      return 0;
    }
    sqlite3_stmt *drop = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM gain WHERE path = ?1 AND begin_ms = ?2", -1, &drop,
                           nullptr) != SQLITE_OK) {
      sqlite3_finalize(list);
      return 0;
    }

    int removed = 0;
    exec("BEGIN");
    for (const QString &folder : folders) {
      if (!QFileInfo::exists(folder)) {
        continue; // the whole folder is gone: an unmounted drive, not a deletion
      }
      // '/' + 1 == '0', so [folder/, folder0) is exactly the folder's subtree
      QVector<Key> gone;
      sqlite3_reset(list);
      bindText(list, 1, folder + QChar('/'));
      bindText(list, 2, folder + QChar('0'));
      while (sqlite3_step(list) == SQLITE_ROW) {
        const QString path =
            QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(list, 0)));
        // only this folder's own files, not those of its subfolders
        if (QFileInfo(path).absolutePath() == folder && !QFileInfo::exists(path)) {
          gone.append(Key(path, quint64(sqlite3_column_int64(list, 1))));
        }
      }

      for (const auto &key : gone) {
        sqlite3_reset(drop);
        bindText(drop, 1, key.first);
        sqlite3_bind_int64(drop, 2, sqlite3_int64(key.second));
        if (sqlite3_step(drop) == SQLITE_DONE) {
          removed += sqlite3_changes(db);
        }
      }
    }
    exec("COMMIT");

    sqlite3_finalize(list);
    sqlite3_finalize(drop);
    return removed;
  }
}
