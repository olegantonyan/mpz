#ifndef REPLAYGAIN_TAGS_H
#define REPLAYGAIN_TAGS_H

#include "replaygain/gain.h"

#include <QString>
#include <tpropertymap.h>

namespace ReplayGain {
  enum class TagResult { Ok, Unsupported, OpenFailed, SaveFailed };

  Gain fromProperties(const TagLib::PropertyMap &props, bool opus);

  Gain readTags(const QString &path);

  TagResult writeTags(const QString &path, const Gain &gain);
}

#endif // REPLAYGAIN_TAGS_H
