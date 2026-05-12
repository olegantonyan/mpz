#include "lyrics/lrcparser.h"

#include <QRegularExpression>
#include <QStringList>

namespace Lyrics {
  bool LrcParser::looksLikeLrc(const QString &raw) {
    static const QRegularExpression timestamp_re(QStringLiteral("\\[\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?\\]"));
    return timestamp_re.match(raw).hasMatch();
  }

  QString LrcParser::stripTimestamps(const QString &raw) {
    static const QRegularExpression timestamp_re(QStringLiteral("\\[\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?\\]"));
    static const QRegularExpression metadata_re(QStringLiteral("^\\[[a-zA-Z]{2,}:[^\\]]*\\]\\s*$"));
    QStringList out;
    const auto lines = raw.split('\n');
    for (const auto &line : lines) {
      QString trimmed = line;
      trimmed.remove('\r');
      if (metadata_re.match(trimmed).hasMatch()) {
        continue;
      }
      QString cleaned = trimmed;
      cleaned.remove(timestamp_re);
      out << cleaned.trimmed();
    }
    while (!out.isEmpty() && out.first().isEmpty()) {
      out.removeFirst();
    }
    while (!out.isEmpty() && out.last().isEmpty()) {
      out.removeLast();
    }
    return out.join('\n');
  }
}
