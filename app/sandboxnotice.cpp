#include "sandboxnotice.h"

namespace {
  const QString notice_url = "https://github.com/olegantonyan/mpz#flatpak";
}

QString sandboxNoticeText() {
  return QString("This Flatpak build has limited filesystem access. <a href=\"%1\">Learn more</a>").arg(notice_url);
}
