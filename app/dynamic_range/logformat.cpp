#include "dynamic_range/logformat.h"

#include <QHash>
#include <QStringList>

namespace DynamicRange {
  namespace {
    constexpr int LINE_WIDTH = 80;
    constexpr double INF_THRESHOLD_DB = -120.0;
    const QString METER_VERSION = QStringLiteral("1.0");
    const QString NOT_AVAILABLE = QStringLiteral("n/a");
    const QString VARIOUS = QStringLiteral("various");

    // deliberately not aligned with the data columns below - foobar2000 emits it this way
    const QString COLUMN_HEADER = QStringLiteral("DR         Peak         RMS     Duration Track");

    QString separator(QChar c) {
      return QString(LINE_WIDTH, c);
    }

    QString mmss(quint32 ms) {
      const quint64 total = ms / 1000;
      const quint64 hours = total / 3600;
      const quint64 minutes = (total % 3600) / 60;
      const quint64 seconds = total % 60;
      if (hours > 0) {
        return QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
      }
      return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
    }

    QString db(double value) {
      if (value <= INF_THRESHOLD_DB) {
        return QStringLiteral("-inf dB");
      }
      return QString("%1 dB").arg(QString::number(value, 'f', 2));
    }

    QString ranges(const QVector<int> &indexes) {
      QStringList parts;
      int i = 0;
      while (i < indexes.size()) {
        int j = i;
        while (j + 1 < indexes.size() && indexes.at(j + 1) == indexes.at(j) + 1) {
          ++j;
        }
        parts << (i == j ? QString::number(indexes.at(i))
                         : QString("%1-%2").arg(indexes.at(i)).arg(indexes.at(j)));
        i = j + 1;
      }
      return parts.join(',');
    }

    QStringList analyzed(const QVector<Entry> &entries) {
      QStringList keys;
      QHash<QString, QVector<int>> by_key;
      for (int i = 0; i < entries.size(); ++i) {
        const QString key = entries.at(i).artist + " / " + entries.at(i).album;
        if (!by_key.contains(key)) {
          keys << key;
        }
        by_key[key] << i + 1;
      }

      QStringList lines;
      for (const auto &key : std::as_const(keys)) {
        lines << QString("%1 (%2)").arg(key, ranges(by_key.value(key)));
      }
      return lines;
    }

    template <typename F>
    QString uniform(const QVector<Entry> &entries, F extract) {
      QString first;
      bool started = false;
      for (const auto &e : entries) {
        const QString value = extract(e);
        if (!started) {
          first = value;
          started = true;
        } else if (value != first) {
          return VARIOUS;
        }
      }
      return first;
    }

    QString row(const Entry &e) {
      const bool ok = e.result.valid;
      const QString dr = ok ? QString("DR%1").arg(displayDr(e.result)) : NOT_AVAILABLE;
      return dr.leftJustified(10) +
             (ok ? db(e.result.peak_db) : NOT_AVAILABLE).rightJustified(8) +
             (ok ? db(e.result.rms_db) : NOT_AVAILABLE).rightJustified(12) +
             mmss(e.duration_ms).rightJustified(10) + " " + e.display;
    }

    QString summaryLine(const QString &label, const QString &value) {
      return label.leftJustified(19) + value;
    }
  }

  QString formatLog(const QVector<Entry> &entries, const LogMeta &meta) {
    QVector<Result> results;
    results.reserve(entries.size());
    for (const auto &e : entries) {
      results << e.result;
    }
    int analyzed_count = 0;
    for (const auto &e : entries) {
      if (e.result.valid) {
        ++analyzed_count;
      }
    }

    QStringList lines;
    lines << QString("mpz %1 / Dynamic Range Meter %2").arg(meta.app_version, METER_VERSION);
    lines << QString("log date: %1").arg(meta.when.toString("yyyy-MM-dd HH:mm:ss"));
    lines << separator('-');

    const QStringList groups = analyzed(entries);
    for (int i = 0; i < groups.size(); ++i) {
      lines << (i == 0 ? QString("Analyzed: %1").arg(groups.at(i))
                       : QString(10, ' ') + groups.at(i));
    }

    lines << separator('-');
    lines << COLUMN_HEADER;
    lines << separator('-');
    for (const auto &e : entries) {
      lines << row(e);
    }
    lines << separator('-');

    lines << summaryLine("Number of tracks:", QString::number(analyzed_count));
    lines << summaryLine("Official DR value:", QString("DR%1").arg(officialAlbumDr(results)));
    lines << summaryLine("Samplerate:", uniform(entries, [](const Entry &e) {
      return QString("%1 Hz").arg(e.sample_rate);
    }));
    lines << summaryLine("Channels:", uniform(entries, [](const Entry &e) {
      return QString::number(e.channels);
    }));
    lines << summaryLine("Bits per sample:", uniform(entries, [](const Entry &e) {
      return e.bits_per_sample > 0 ? QString::number(e.bits_per_sample) : NOT_AVAILABLE;
    }));
    lines << summaryLine("Bitrate:", uniform(entries, [](const Entry &e) {
      return QString("%1 kbps").arg(e.bitrate);
    }));
    lines << summaryLine("Codec:", uniform(entries, [](const Entry &e) {
      return e.codec;
    }));
    lines << separator('=');

    return lines.join('\n') + '\n';
  }
}
