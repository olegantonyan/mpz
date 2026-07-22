#include "autoeqimport.h"

#include <QRegularExpression>
#include <QStringList>

namespace Eq {
  namespace {
    bool mapType(const QString &token, Band::Type &out) {
      const QString t = token.toUpper();
      if (t == "PK" || t == "PEQ" || t == "MODAL") {
        out = Band::Type::Peaking;
      } else if (t == "LS" || t == "LSC" || t == "LSQ" || t == "LOWSHELF") {
        out = Band::Type::LowShelf;
      } else if (t == "HS" || t == "HSC" || t == "HSQ" || t == "HIGHSHELF") {
        out = Band::Type::HighShelf;
      } else if (t == "LP" || t == "LPQ" || t == "LPF") {
        out = Band::Type::LowPass;
      } else if (t == "HP" || t == "HPQ" || t == "HPF") {
        out = Band::Type::HighPass;
      } else {
        return false;
      }
      return true;
    }
  }

  ParsedEq parseParametricEq(const QString &text) {
    ParsedEq result;

    static const QRegularExpression preamp_re(
      "^\\s*Preamp:\\s*(-?[0-9]*\\.?[0-9]+)\\s*dB",
      QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression filter_re(
      "^\\s*Filter\\s*[0-9]+:\\s*(ON|OFF)\\s+([A-Za-z]+)",
      QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression fc_re(
      "\\bFc\\s+(-?[0-9]*\\.?[0-9]+)\\s*Hz", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression gain_re(
      "\\bGain\\s+(-?[0-9]*\\.?[0-9]+)\\s*dB", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression q_re(
      "\\bQ\\s+(-?[0-9]*\\.?[0-9]+)", QRegularExpression::CaseInsensitiveOption);

    const QStringList lines = text.split('\n');
    for (const QString &raw : lines) {
      const QString line = raw.trimmed();
      if (line.isEmpty()) {
        continue;
      }

      const auto pm = preamp_re.match(line);
      if (pm.hasMatch()) {
        result.preamp_present = true;
        result.preamp_db = pm.captured(1).toDouble();
        continue;
      }

      const auto fm = filter_re.match(line);
      if (!fm.hasMatch()) {
        continue;
      }

      Band::Type type;
      if (!mapType(fm.captured(2), type)) {
        continue;
      }

      const auto fc = fc_re.match(line);
      if (!fc.hasMatch()) {
        continue;
      }

      Band b;
      b.type = type;
      b.enabled = fm.captured(1).compare("OFF", Qt::CaseInsensitive) != 0;
      b.freq_hz = fc.captured(1).toDouble();

      const auto gm = gain_re.match(line);
      b.gain_db = gm.hasMatch() ? gm.captured(1).toDouble() : 0.0;

      const auto qm = q_re.match(line);
      b.q = qm.hasMatch() ? qm.captured(1).toDouble() : 0.707;
      if (b.q <= 0.0) {
        b.q = 0.707;
      }

      result.bands.push_back(b);
    }

    return result;
  }
}
