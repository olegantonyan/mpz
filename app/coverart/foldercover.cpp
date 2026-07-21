#include "coverart/foldercover.h"

#include <QFileInfo>
#include <QRegularExpression>

namespace CoverArt {
  namespace FolderCover {
    namespace {
      struct Rule {
        const char *word;
        int weight;
      };

      const Rule COMPACT_RULES[] = {
        {"front", 100}, {"folder", 80}, {"albumart", 80}, {"cover", 60},
        {"artwork", 40}, {"large", 10},
        {"back", -120}, {"rear", -120}, {"inside", -100}, {"inner", -100},
        {"inlay", -100}, {"insert", -90}, {"booklet", -90}, {"disc", -90},
        {"disk", -90}, {"tray", -80}, {"spine", -80}, {"logo", -80},
        {"matrix", -70}, {"sticker", -70}, {"poster", -50}, {"artist", -50},
        {"photo", -50}, {"scan", -30}, {"small", -20},
      };

      const Rule TOKEN_RULES[] = {
        {"album", 30},
        {"cd", -60}, {"dvd", -60}, {"obi", -70}, {"band", -60}, {"book", -60},
        {"lp", -50}, {"side", -40}, {"thumb", -40},
      };
    }

    int score(const QString &fileName) {
      QString stem = QFileInfo(fileName).completeBaseName();
      static const QRegularExpression camel("([a-z0-9])([A-Z])");
      stem.replace(camel, "\\1 \\2");
      stem = stem.toLower();

      static const QRegularExpression sep("[^a-z0-9]+");
      const QStringList tokens = stem.split(sep);
      const QString compact = tokens.join(QString());

      int total = 0;
      for (const auto &r : COMPACT_RULES) {
        if (compact.contains(QLatin1String(r.word))) {
          total += r.weight;
        }
      }
      for (const auto &r : TOKEN_RULES) {
        if (tokens.contains(QLatin1String(r.word))) {
          total += r.weight;
        }
      }
      return total;
    }

    Match best(const QStringList &fileNames) {
      Match result;
      bool found = false;
      for (const auto &name : fileNames) {
        const int s = score(name);
        const bool better = !found || s > result.score ||
          (s == result.score &&
           (name.size() < result.file.size() ||
            (name.size() == result.file.size() &&
             name.compare(result.file, Qt::CaseInsensitive) < 0)));
        if (better) {
          result.file = name;
          result.score = s;
          found = true;
        }
      }
      return result;
    }
  }
}
