#ifndef TAGLIB_COMPAT_H
#define TAGLIB_COMPAT_H

#include <taglib.h>

#define MPZ_TAGLIB_SINCE(major, minor) \
  (TAGLIB_MAJOR_VERSION > (major) || (TAGLIB_MAJOR_VERSION == (major) && TAGLIB_MINOR_VERSION >= (minor)))

#endif // TAGLIB_COMPAT_H
