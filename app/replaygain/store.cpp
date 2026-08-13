#include "replaygain/store.h"

#include <algorithm>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

namespace ReplayGain {
  namespace {
    const char kHeader[] = "#mpz-rg 1";
    const int kVersion = 1;
    const int kFieldCount = 8;
    const double kCompactRatio = 0.3;

    QString formatGain(double db, bool present) {
      return present ? QString::number(db, 'f', 2) : QString();
    }

    QString formatPeak(double peak, bool present) {
      return present ? QString::number(peak, 'f', 6) : QString();
    }

    bool parseOptional(const QString &field, double *out) {
      if (field.isEmpty()) {
        return false;
      }
      bool ok = false;
      const double v = field.toDouble(&ok);
      if (ok) {
        *out = v;
      }
      return ok;
    }

    QString escape(const QString &s) {
      QString r;
      r.reserve(s.size());
      for (const QChar c : s) {
        switch (c.unicode()) {
          case '\\': r += QLatin1String("\\\\"); break;
          case '\t': r += QLatin1String("\\t"); break;
          case '\n': r += QLatin1String("\\n"); break;
          case '\r': r += QLatin1String("\\r"); break;
          default: r += c; break;
        }
      }
      return r;
    }

    QString unescape(const QString &s) {
      QString r;
      r.reserve(s.size());
      for (int i = 0; i < s.size(); i++) {
        if (s.at(i) != QChar('\\') || i + 1 >= s.size()) {
          r += s.at(i);
          continue;
        }
        i++;
        switch (s.at(i).unicode()) {
          case 't': r += QChar('\t'); break;
          case 'n': r += QChar('\n'); break;
          case 'r': r += QChar('\r'); break;
          default: r += s.at(i); break;
        }
      }
      return r;
    }

    QString formatLine(const Store::Entry &e) {
      return QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t%8")
          .arg(formatGain(e.gain.track_db, e.gain.has_track),
               formatPeak(e.gain.track_peak, e.gain.has_track),
               formatGain(e.gain.album_db, e.gain.has_album),
               formatPeak(e.gain.album_peak, e.gain.has_album),
               QString::number(e.mtime),
               QString::number(e.size),
               QString::number(e.begin_ms),
               escape(e.path));
    }
  }

  Store::Store(const QString &dir) {
    QDir().mkpath(dir);
    filepath = QString("%1/replaygain.db").arg(dir);
    out.setFileName(filepath);
    reload();
    if (needs_rewrite) {
      compact();
    }
  }

  Store::~Store() {
    closeOutput();
  }

  bool Store::reload() {
    entries.clear();
    live_lines = 0;
    dead_lines = 0;
    needs_rewrite = false;

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
      return false;
    }

    bool header_seen = false;
    while (!file.atEnd()) {
      QByteArray raw = file.readLine();
      if (raw.endsWith('\n')) {
        raw.chop(1);
      }
      QString line = QString::fromUtf8(raw);
      if (line.endsWith(QChar('\r'))) {
        line.chop(1);
      }
      if (line.isEmpty()) {
        continue;
      }
      if (line.startsWith(QChar('#'))) {
        if (!header_seen) {
          header_seen = true;
          if (line.split(QChar(' ')).value(1).toInt() != kVersion) {
            entries.clear();
            needs_rewrite = true;
            return false;
          }
        }
        continue;
      }

      const QStringList fields = line.split(QChar('\t'));
      if (fields.size() != kFieldCount) {
        dead_lines++;
        continue;
      }

      bool ok = false;
      Entry e;
      e.mtime = fields.at(4).toLongLong(&ok);
      if (!ok) {
        dead_lines++;
        continue;
      }
      e.size = fields.at(5).toLongLong(&ok);
      if (!ok) {
        dead_lines++;
        continue;
      }
      e.begin_ms = fields.at(6).toULongLong(&ok);
      if (!ok) {
        dead_lines++;
        continue;
      }
      e.path = unescape(fields.at(7));
      if (e.path.isEmpty()) {
        dead_lines++;
        continue;
      }

      e.gain.has_track = parseOptional(fields.at(0), &e.gain.track_db);
      parseOptional(fields.at(1), &e.gain.track_peak);
      e.gain.has_album = parseOptional(fields.at(2), &e.gain.album_db);
      parseOptional(fields.at(3), &e.gain.album_peak);
      if (!e.gain.isValid()) {
        dead_lines++;
        continue;
      }

      const Key k(e.path, e.begin_ms);
      if (entries.contains(k)) {
        dead_lines++;
      } else {
        live_lines++;
      }
      entries.insert(k, e);
    }

    return true;
  }

  Gain Store::get(const QString &path, quint64 begin_ms) const {
    return get(path, QFileInfo(path), begin_ms);
  }

  Gain Store::get(const QString &path, const QFileInfo &info, quint64 begin_ms) const {
    const auto it = entries.constFind(Key(path, begin_ms));
    if (it == entries.constEnd()) {
      return Gain();
    }
    if (!info.exists() || info.lastModified().toSecsSinceEpoch() != it->mtime || info.size() != it->size) {
      return Gain();
    }
    return it->gain;
  }

  bool Store::put(const QString &path, quint64 begin_ms, const Gain &gain) {
    if (path.isEmpty() || !gain.isValid()) {
      return false;
    }
    const QFileInfo info(path);
    if (!info.exists()) {
      return false;
    }

    Entry e;
    e.path = path;
    e.begin_ms = begin_ms;
    e.gain = gain;
    e.mtime = info.lastModified().toSecsSinceEpoch();
    e.size = info.size();

    if (!appendLine(formatLine(e))) {
      return false;
    }

    const Key k(path, begin_ms);
    if (entries.contains(k)) {
      dead_lines++;
    } else {
      live_lines++;
    }
    entries.insert(k, e);
    return true;
  }

  bool Store::appendLine(const QString &line) {
    if (!out.isOpen()) {
      if (!out.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return false;
      }
      if (out.size() == 0) {
        out.write(kHeader);
        out.write("\n");
      }
    }
    const QByteArray payload = line.toUtf8() + '\n';
    return out.write(payload) == payload.size() && out.flush();
  }

  bool Store::compact() {
    closeOutput();

    QList<Key> keys = entries.keys();
    std::sort(keys.begin(), keys.end());

    QHash<Key, Entry> kept;
    kept.reserve(entries.size());
    QByteArray payload = QByteArray(kHeader) + '\n';
    QString last_dir;
    bool last_dir_exists = false;

    for (const auto &k : keys) {
      const Entry e = entries.value(k);
      const QFileInfo info(e.path);
      if (!info.exists()) {
        const QString dir = info.absolutePath();
        if (dir != last_dir) {
          last_dir = dir;
          last_dir_exists = QFileInfo::exists(dir);
        }
        // an unmounted drive must not wipe the db
        if (last_dir_exists) {
          continue;
        }
      }
      payload += formatLine(e).toUtf8() + '\n';
      kept.insert(k, e);
    }

    QSaveFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
      return false;
    }
    if (file.write(payload) != payload.size() || !file.commit()) {
      return false;
    }

    entries = std::move(kept);
    live_lines = entries.size();
    dead_lines = 0;
    needs_rewrite = false;
    return true;
  }

  void Store::compactIfNeeded() {
    if (live_lines > 0 && dead_lines > live_lines * kCompactRatio) {
      compact();
    }
  }

  void Store::closeOutput() {
    if (out.isOpen()) {
      out.close();
    }
  }
}
