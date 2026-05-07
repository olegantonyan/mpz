#include "cueparser.h"

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
  #include <QRegularExpression>
  #include <QStringConverter>
  #include <QStringDecoder>
#else
  #include <QRegExp>
  #include <QTextCodec>
#endif

namespace Playlist {
  namespace {
    // Windows-1251 (CP1251, Cyrillic) → Unicode for bytes 0x80..0xFF.
    // Used as a fallback when a cue isn't valid UTF-8; common for Russian rips.
    static const ushort kCp1251Hi[128] = {
      0x0402,0x0403,0x201A,0x0453,0x201E,0x2026,0x2020,0x2021,
      0x20AC,0x2030,0x0409,0x2039,0x040A,0x040C,0x040B,0x040F,
      0x0452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
      0x0000,0x2122,0x0459,0x203A,0x045A,0x045C,0x045B,0x045F,
      0x00A0,0x040E,0x045E,0x0408,0x00A4,0x0490,0x00A6,0x00A7,
      0x0401,0x00A9,0x0404,0x00AB,0x00AC,0x00AD,0x00AE,0x0407,
      0x00B0,0x00B1,0x0406,0x0456,0x0491,0x00B5,0x00B6,0x00B7,
      0x0451,0x2116,0x0454,0x00BB,0x0458,0x0405,0x0455,0x0457,
      0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
      0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
      0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
      0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
      0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
      0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
      0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
      0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F
    };

    QString decode_cp1251(const QByteArray& bytes) {
      QString out;
      out.reserve(bytes.size());
      for (auto b : bytes) {
        const unsigned char c = static_cast<unsigned char>(b);
        if (c < 0x80) {
          out.append(QChar(c));
        } else {
          out.append(QChar(kCp1251Hi[c - 0x80]));
        }
      }
      return out;
    }

    QString decode_bytes(const QByteArray& bytes) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
      // Try BOM-based detection first.
      if (auto enc = QStringConverter::encodingForData(bytes)) {
        QStringDecoder dec(*enc);
        QString s = dec.decode(bytes);
        if (!dec.hasError()) {
          return s;
        }
      }
      // Strict UTF-8.
      {
        QStringDecoder dec(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
        QString s = dec.decode(bytes);
        if (!dec.hasError()) {
          return s;
        }
      }
      // Cue files in the wild often use Windows-1251 (Russian) or Latin-1.
      // CP1251 covers more cases in practice (and degrades gracefully to
      // Latin-1-like for Western text — the affected bytes are usually only
      // in metadata strings, which we won't try to fix beyond best effort).
      return decode_cp1251(bytes);
#else
      QTextCodec* codec = QTextCodec::codecForUtfText(bytes, QTextCodec::codecForName("UTF-8"));
      return codec ? codec->toUnicode(bytes) : QString::fromUtf8(bytes);
#endif
    }

    QStringList read_lines(const QString& path) {
      QFile f(path);
      if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "cueparser: cannot open" << path << ":" << f.errorString();
        return {};
      }
      QString text = decode_bytes(f.readAll());
      // Strip a leading BOM that decoders may have left in.
      if (!text.isEmpty() && text.at(0) == QChar(0xFEFF)) {
        text.remove(0, 1);
      }
      // Normalize line endings.
      text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
      text.replace(QChar('\r'), QChar('\n'));
      return text.split(QChar('\n'));
    }
  } // namespace

  CueParser::CueParser(const QString& p) : path_(p) {
  }

  // Splits a single CUE line into command + values. Handles:
  //   - leading/trailing whitespace
  //   - comment lines starting with ';'
  //   - quoted values that may contain whitespace
  //   - any number of unquoted whitespace-separated values
  QStringList CueParser::tokenize(const QString& raw) {
    QString s = raw;
    // Trim. Don't use trimmed() because we want to skip leading BOMs too.
    int start = 0;
    while (start < s.size() && (s.at(start).isSpace() || s.at(start) == QChar(0xFEFF))) {
      ++start;
    }
    int end = s.size();
    while (end > start && s.at(end - 1).isSpace()) {
      --end;
    }
    if (start >= end) {
      return {};
    }
    s = s.mid(start, end - start);
    if (s.startsWith(QChar(';'))) {
      return {};
    }

    QStringList tokens;
    int i = 0;
    const int n = s.size();
    while (i < n) {
      while (i < n && s.at(i).isSpace()) {
        ++i;
      }
      if (i >= n) {
        break;
      }
      if (s.at(i) == QChar('"')) {
        ++i;
        int q_start = i;
        while (i < n && s.at(i) != QChar('"')) {
          ++i;
        }
        tokens << s.mid(q_start, i - q_start);
        if (i < n) {
          ++i; // consume closing quote
        }
      } else {
        int w_start = i;
        while (i < n && !s.at(i).isSpace()) {
          ++i;
        }
        tokens << s.mid(w_start, i - w_start);
      }
    }
    return tokens;
  }

  // Parse "MM:SS:FF" (minutes:seconds:frames, 75 frames per second) into ms.
  // Returns -1 if the input doesn't look like an INDEX position.
  qint64 CueParser::parse_index_position(const QString& mss_ff) {
    const auto parts = mss_ff.split(QChar(':'));
    if (parts.size() != 3) {
      return -1;
    }
    bool ok1 = false, ok2 = false, ok3 = false;
    qint64 m = parts.at(0).toLongLong(&ok1);
    qint64 s = parts.at(1).toLongLong(&ok2);
    qint64 f = parts.at(2).toLongLong(&ok3);
    if (!ok1 || !ok2 || !ok3 || m < 0 || s < 0 || f < 0) {
      return -1;
    }
    qint64 frames = m * 60 * 75 + s * 75 + f;
    return (frames * 1000) / 75;
  }

  // Resolve the FILE reference from the cue against the cue's directory.
  // Strategy, in order:
  //   1. normalize Windows backslashes on Unix
  //   2. exact path (canonicalized) if it exists
  //   3. case-insensitive name match in the parent directory
  //   4. stem match against any audio file in the parent directory — handles
  //      foobar2000-style "stub WAV name for FLAC" cues where the cue says
  //      "Track.wav" but the actual file is "Track.flac" / ".ape" / ".mp3"
  //   5. fallback to the same trick using only the basename in cue_dir
  //      (some cues prefix the name with subdirectories that don't exist)
  //   6. give up: return empty so the caller can skip the FILE block
  QString CueParser::resolve_audio_file(const QString& cue_dir, const QString& referenced) {
    if (referenced.isEmpty()) {
      return {};
    }
    QString r = referenced;
#ifndef Q_OS_WIN
    r.replace(QChar('\\'), QChar('/'));
#endif
    const QString abs = QDir::isAbsolutePath(r) ? r : QDir(cue_dir).absoluteFilePath(r);

    {
      const QString canon = QFileInfo(abs).canonicalFilePath();
      if (!canon.isEmpty()) {
        return canon;
      }
    }

    static const QStringList kAudioExt = {
      "flac", "ape", "wav", "wv", "mp3", "m4a", "mp4", "ogg",
      "opus", "dsf", "aac", "wma", "tta", "tak", "alac"
    };

    auto find_in_dir = [](const QDir& dir, const QString& want_name) -> QString {
      if (!dir.exists()) {
        return {};
      }
      const QFileInfo want(want_name);
      const QString want_full = want.fileName();
      const QString want_stem = want.completeBaseName();
      const QStringList entries = dir.entryList(QDir::Files);
      // Pass 1: exact name (case-insensitive).
      for (const QString& e : entries) {
        if (e.compare(want_full, Qt::CaseInsensitive) == 0) {
          return dir.absoluteFilePath(e);
        }
      }
      // Pass 2: same stem with a known audio extension.
      for (const QString& e : entries) {
        const QFileInfo efi(e);
        if (efi.completeBaseName().compare(want_stem, Qt::CaseInsensitive) == 0
            && kAudioExt.contains(efi.suffix().toLower())) {
          return dir.absoluteFilePath(e);
        }
      }
      return {};
    };

    if (QString found = find_in_dir(QDir(QFileInfo(abs).absolutePath()), QFileInfo(abs).fileName());
        !found.isEmpty()) {
      return found;
    }

    // Some cues reference paths like "DJ Tiësto/Traffic/01-foo.wav" that don't
    // resolve. Try looking for the basename (with stem fallback) directly in
    // the cue's directory.
    if (QString found = find_in_dir(QDir(cue_dir), QFileInfo(r).fileName()); !found.isEmpty()) {
      return found;
    }

    return {};
  }

  quint16 CueParser::parse_year(const QString& s) {
    if (s.isEmpty()) {
      return 0;
    }
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QRegularExpression re(QStringLiteral("\\d{4}"));
    auto m = re.match(s);
    return m.hasMatch() ? m.captured(0).toUShort() : 0;
#else
    QRegExp re("\\d{4}");
    int idx = re.indexIn(s);
    return idx >= 0 ? re.cap(0).toUShort() : 0;
#endif
  }

  QVector<Track> CueParser::tracks_list() const {
    const QStringList lines = read_lines(path_);
    if (lines.isEmpty()) {
      return {};
    }

    const QString cue_dir = QFileInfo(path_).absoluteDir().absolutePath();

    // Album-level (set before any TRACK).
    QString album_artist;
    QString album_title;
    QString album_composer;
    QString album_genre;
    QString album_date;
    QString album_disc;

    // Current FILE.
    QString cur_file;
    bool cur_file_audio = true; // false for BINARY/MOTOROLA — skip its tracks

    // Current TRACK.
    Entry cur;
    bool in_track = false;
    bool cur_audio = true; // false for non-AUDIO TRACK types

    QList<Entry> entries;

    auto flush_track = [&]() {
      if (in_track && cur_file_audio && cur_audio && !cur_file.isEmpty()) {
        const qint64 begin = cur.index01_ms >= 0 ? cur.index01_ms : cur.index00_ms;
        if (begin >= 0) {
          cur.file = cur_file;
          entries.append(cur);
        }
      }
      cur = Entry{};
      in_track = false;
      cur_audio = true;
    };

    for (const QString& raw : lines) {
      const QStringList toks = tokenize(raw);
      if (toks.isEmpty()) {
        continue;
      }
      const QString cmd = toks.at(0).toLower();

      if (cmd == QLatin1String("file")) {
        flush_track();
        const QString name = toks.value(1);
        const QString type = toks.value(2);
        cur_file = resolve_audio_file(cue_dir, name);
        // BINARY/MOTOROLA = data tracks. Skip everything under them.
        cur_file_audio = !(type.compare(QLatin1String("BINARY"), Qt::CaseInsensitive) == 0
                           || type.compare(QLatin1String("MOTOROLA"), Qt::CaseInsensitive) == 0);
      } else if (cmd == QLatin1String("track")) {
        flush_track();
        in_track = true;
        cur.track_no = toks.value(1).toInt();
        const QString type = toks.value(2);
        cur_audio = type.isEmpty() || type.compare(QLatin1String("AUDIO"), Qt::CaseInsensitive) == 0;
      } else if (cmd == QLatin1String("index")) {
        if (!in_track) {
          continue;
        }
        const int idx = toks.value(1).toInt();
        const qint64 t = parse_index_position(toks.value(2));
        if (t < 0) {
          continue;
        }
        if (idx == 0) {
          cur.index00_ms = t;
        } else if (idx == 1) {
          cur.index01_ms = t;
        }
      } else if (cmd == QLatin1String("performer")) {
        const QString v = toks.value(1);
        if (in_track) {
          cur.artist = v;
        } else {
          album_artist = v;
        }
      } else if (cmd == QLatin1String("title")) {
        const QString v = toks.value(1);
        if (in_track) {
          cur.title = v;
        } else {
          album_title = v;
        }
      } else if (cmd == QLatin1String("songwriter")) {
        const QString v = toks.value(1);
        if (in_track) {
          cur.composer = v;
        } else {
          album_composer = v;
        }
      } else if (cmd == QLatin1String("rem")) {
        if (toks.size() < 3) {
          continue;
        }
        const QString sub = toks.at(1).toLower();
        const QString v = toks.at(2);
        if (sub == QLatin1String("genre")) {
          album_genre = v;
        } else if (sub == QLatin1String("date")) {
          album_date = v;
        } else if (sub == QLatin1String("discnumber") || sub == QLatin1String("disc")) {
          album_disc = v;
        }
      }
      // All other commands (CATALOG, ISRC, FLAGS, CDTEXTFILE, POSTGAP, PREGAP, ...)
      // are ignored on purpose.
    }
    flush_track();

    if (entries.isEmpty()) {
      qWarning() << "cueparser:" << path_ << "produced no tracks";
      return {};
    }

    Q_UNUSED(album_composer);
    Q_UNUSED(album_genre);
    Q_UNUSED(album_disc);

    const quint16 year = parse_year(album_date);

    QVector<Track> tracks;
    tracks.reserve(entries.size());

    for (int i = 0; i < entries.size(); ++i) {
      const Entry& e = entries.at(i);
      const qint64 begin = e.index01_ms >= 0 ? e.index01_ms : (e.index00_ms >= 0 ? e.index00_ms : 0);

      Track track(e.file,
                  static_cast<quint64>(begin),
                  e.artist.isEmpty() ? album_artist : e.artist,
                  album_title,
                  e.title,
                  static_cast<quint16>(e.track_no > 0 ? e.track_no : i + 1),
                  year,
                  0, 0, 0, 0);
      track.fillAudioProperties();
      track.setCue();

      qint64 duration_ms = -1;
      if (i + 1 < entries.size() && entries.at(i + 1).file == e.file) {
        const Entry& next = entries.at(i + 1);
        const qint64 next_begin = next.index01_ms >= 0 ? next.index01_ms
                                                       : (next.index00_ms >= 0 ? next.index00_ms : -1);
        if (next_begin >= 0) {
          duration_ms = next_begin - begin;
        }
      }
      if (duration_ms < 0) {
        // Last track for this file: use audio file length.
        duration_ms = static_cast<qint64>(track.duration()) - begin;
      }
      track.setDuration(duration_ms < 0 ? 0 : static_cast<quint64>(duration_ms));

      tracks << track;
    }

    return tracks;
  }
}
