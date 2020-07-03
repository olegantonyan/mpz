TEMPLATE = lib
TARGET = cuefile

CONFIG += warn_off
CONFIG += staticlib

SOURCES += \
  libcuefile_r475/src/cd.c \
  libcuefile_r475/src/cdtext.c \
  libcuefile_r475/src/cue_parse.c \
  libcuefile_r475/src/cue_print.c \
  libcuefile_r475/src/cue_scan.c \
  libcuefile_r475/src/time.c \
  libcuefile_r475/src/toc_parse.c \
  libcuefile_r475/src/toc_print.c \
  libcuefile_r475/src/toc_scan.c

HEADERS += \
  libcuefile_r475/src/cue.h \
  libcuefile_r475/src/cue_parse.h \
  libcuefile_r475/src/cue_parse_prefix.h \
  libcuefile_r475/src/time.h \
  libcuefile_r475/src/toc_parse.h \
  libcuefile_r475/src/toc_parse_prefix.h \
  libcuefile_r475/include/cuetools/cd.h \
  libcuefile_r475/include/cuetools/cdtext.h \
  libcuefile_r475/include/cuetools/cuefile.h

INCLUDEPATH += \
  libcuefile_r475/include
