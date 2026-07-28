#ifndef REPLAYGAIN_TAGS_H
#define REPLAYGAIN_TAGS_H

#include "replaygain/gain.h"

#include <QString>

namespace TagLib {
  class PropertyMap;
}

namespace ReplayGain {
  enum class TagResult { Ok, Unsupported, OpenFailed, SaveFailed };

  Gain fromProperties(const TagLib::PropertyMap &props, bool opus);

  Gain readTags(const QString &path);

  TagResult writeTags(const QString &path, const Gain &gain);
}

#endif // REPLAYGAIN_TAGS_H
