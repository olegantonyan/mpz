#include "lyrics/lrcparser.h"

#include <QRegularExpression>
#include <QStringList>

namespace {
  // NetEase appends a "-N" sub-index to some timestamps: [00:00.00-1].
  const QRegularExpression &timestampRe() {
    static const QRegularExpression re(QStringLiteral("\\[\\d{1,3}:\\d{1,2}(?:[.:]\\d{1,3})?(?:-\\d+)?\\]"));
    return re;
  }

  // Production credits NetEase and QQ Music ship instead of, or ahead of, the lyrics. The trailing colon is what keeps sung lines out.
  const QRegularExpression &creditRe() {
    static const QString labels = QStringLiteral(
      "作词|作詞|作曲|编曲|編曲|词|曲|制作人|製作人|出品人|监制|監製|录音|錄音|混音|"
      "母带|母帶|和声|和聲|吉他|贝斯|貝斯|鼓|键盘|鍵盤|弦乐|弦樂|制作|製作|演唱|原唱|"
      "lyrics|lyricist|composer|composed|written|arranger|arranged|producer|produced|"
      "mixed|mixing|mastered|mastering|vocals?|guitars?|bass|drums");
    static const QRegularExpression re(QStringLiteral("^(?:%1)(?:\\s+by)?\\s*[:：]\\s*\\S").arg(labels),
                                       QRegularExpression::CaseInsensitiveOption);
    return re;
  }

  // Placeholder bodies for instrumentals or missing lyrics: "纯音乐，请欣赏" (pure music, enjoy), "暂无歌词" (no lyrics yet).
  const QRegularExpression &placeholderRe() {
    static const QRegularExpression re(QStringLiteral("纯音乐|純音樂|暂无歌词|暫無歌詞|没有填词|沒有填詞"));
    return re;
  }

  bool isNonLyric(const QString &line) {
    const QString trimmed = line.trimmed();
    return creditRe().match(trimmed).hasMatch() || placeholderRe().match(trimmed).hasMatch();
  }

  QString joinBody(QStringList lines) {
    while (!lines.isEmpty() && lines.first().trimmed().isEmpty()) {
      lines.removeFirst();
    }
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
      lines.removeLast();
    }
    return lines.join('\n');
  }
}

namespace Lyrics {
  bool LrcParser::looksLikeLrc(const QString &raw) {
    return timestampRe().match(raw).hasMatch();
  }

  QString LrcParser::stripTimestamps(const QString &raw) {
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
      cleaned.remove(timestampRe());
      cleaned = cleaned.trimmed();
      if (isNonLyric(cleaned)) {
        continue;
      }
      out << cleaned;
    }
    return joinBody(out);
  }

  QString LrcParser::toPlainLyrics(const QString &raw) {
    if (looksLikeLrc(raw)) {
      return stripTimestamps(raw);
    }
    QStringList out;
    const auto lines = raw.split('\n');
    for (const auto &line : lines) {
      QString cleaned = line;
      cleaned.remove('\r');
      if (isNonLyric(cleaned)) {
        continue;
      }
      out << cleaned;
    }
    return joinBody(out).trimmed();
  }

  bool LrcParser::hasLyricContent(const QString &raw) {
    return !toPlainLyrics(raw).isEmpty();
  }
}
