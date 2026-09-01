#ifndef TEXTELIDE_H
#define TEXTELIDE_H

#include <QString>

namespace Text {

inline QString elide(const QString &s, int limit) {
  return s.size() <= limit ? s : s.left(limit) + QChar(0x2026);
}

}

#endif // TEXTELIDE_H
