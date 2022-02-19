TEMPLATE = lib
TARGET = taglib

CONFIG += warn_off
CONFIG += staticlib
CONFIG += c++11

DEFINES += "NDEBUG=1"
DEFINES += TAGLIB_STATIC
DEFINES += HAVE_GCC_ATOMIC

win32: CONFIG -= debug_and_release

SOURCES += \
  taglib-1.12/taglib/toolkit/tbytevectorlist.cpp \
  taglib-1.12/taglib/toolkit/tdebug.cpp \
  taglib-1.12/taglib/toolkit/tpropertymap.cpp \
  taglib-1.12/taglib/toolkit/tdebuglistener.cpp \
  taglib-1.12/taglib/toolkit/tstring.cpp \
  taglib-1.12/taglib/toolkit/tbytevectorstream.cpp \
  taglib-1.12/taglib/toolkit/tiostream.cpp \
  taglib-1.12/taglib/toolkit/tstringlist.cpp \
  taglib-1.12/taglib/toolkit/tzlib.cpp \
  taglib-1.12/taglib/toolkit/tbytevector.cpp \
  taglib-1.12/taglib/toolkit/tfilestream.cpp \
  taglib-1.12/taglib/toolkit/trefcounter.cpp \
  taglib-1.12/taglib/toolkit/tfile.cpp \
  taglib-1.12/taglib/xm/xmproperties.cpp \
  taglib-1.12/taglib/xm/xmfile.cpp \
  taglib-1.12/taglib/audioproperties.cpp \
  taglib-1.12/taglib/fileref.cpp \
  taglib-1.12/taglib/trueaudio/trueaudiofile.cpp \
  taglib-1.12/taglib/trueaudio/trueaudioproperties.cpp \
  taglib-1.12/taglib/ape/apetag.cpp \
  taglib-1.12/taglib/ape/apefooter.cpp \
  taglib-1.12/taglib/ape/apefile.cpp \
  taglib-1.12/taglib/ape/apeproperties.cpp \
  taglib-1.12/taglib/ape/apeitem.cpp \
  taglib-1.12/taglib/it/itproperties.cpp \
  taglib-1.12/taglib/it/itfile.cpp \
  taglib-1.12/taglib/riff/aiff/aiffproperties.cpp \
  taglib-1.12/taglib/riff/aiff/aifffile.cpp \
  taglib-1.12/taglib/riff/wav/wavfile.cpp \
  taglib-1.12/taglib/riff/wav/infotag.cpp \
  taglib-1.12/taglib/riff/wav/wavproperties.cpp \
  taglib-1.12/taglib/riff/rifffile.cpp \
  taglib-1.12/taglib/flac/flacunknownmetadatablock.cpp \
  taglib-1.12/taglib/flac/flacpicture.cpp \
  taglib-1.12/taglib/flac/flacmetadatablock.cpp \
  taglib-1.12/taglib/flac/flacfile.cpp \
  taglib-1.12/taglib/flac/flacproperties.cpp \
  taglib-1.12/taglib/mod/modfile.cpp \
  taglib-1.12/taglib/mod/modtag.cpp \
  taglib-1.12/taglib/mod/modproperties.cpp \
  taglib-1.12/taglib/mod/modfilebase.cpp \
  taglib-1.12/taglib/s3m/s3mproperties.cpp \
  taglib-1.12/taglib/s3m/s3mfile.cpp \
  taglib-1.12/taglib/ogg/speex/speexproperties.cpp \
  taglib-1.12/taglib/ogg/speex/speexfile.cpp \
  taglib-1.12/taglib/ogg/oggfile.cpp \
  taglib-1.12/taglib/ogg/flac/oggflacfile.cpp \
  taglib-1.12/taglib/ogg/opus/opusproperties.cpp \
  taglib-1.12/taglib/ogg/opus/opusfile.cpp \
  taglib-1.12/taglib/ogg/oggpageheader.cpp \
  taglib-1.12/taglib/ogg/xiphcomment.cpp \
  taglib-1.12/taglib/ogg/vorbis/vorbisproperties.cpp \
  taglib-1.12/taglib/ogg/vorbis/vorbisfile.cpp \
  taglib-1.12/taglib/ogg/oggpage.cpp \
  taglib-1.12/taglib/tagutils.cpp \
  taglib-1.12/taglib/wavpack/wavpackfile.cpp \
  taglib-1.12/taglib/wavpack/wavpackproperties.cpp \
  taglib-1.12/taglib/mpc/mpcproperties.cpp \
  taglib-1.12/taglib/mpc/mpcfile.cpp \
  taglib-1.12/taglib/asf/asfproperties.cpp \
  taglib-1.12/taglib/asf/asfattribute.cpp \
  taglib-1.12/taglib/asf/asfpicture.cpp \
  taglib-1.12/taglib/asf/asffile.cpp \
  taglib-1.12/taglib/asf/asftag.cpp \
  taglib-1.12/taglib/mp4/mp4file.cpp \
  taglib-1.12/taglib/mp4/mp4tag.cpp \
  taglib-1.12/taglib/mp4/mp4atom.cpp \
  taglib-1.12/taglib/mp4/mp4item.cpp \
  taglib-1.12/taglib/mp4/mp4coverart.cpp \
  taglib-1.12/taglib/mp4/mp4properties.cpp \
  taglib-1.12/taglib/tag.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/tableofcontentsframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/urllinkframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/relativevolumeframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/eventtimingcodesframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/textidentificationframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/chapterframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/uniquefileidentifierframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/attachedpictureframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/synchronizedlyricsframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/ownershipframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/privateframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/podcastframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/commentsframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/unknownframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/frames/popularimeterframe.cpp \
  taglib-1.12/taglib/mpeg/id3v2/id3v2framefactory.cpp \
  taglib-1.12/taglib/mpeg/id3v2/id3v2frame.cpp \
  taglib-1.12/taglib/mpeg/id3v2/id3v2tag.cpp \
  taglib-1.12/taglib/mpeg/id3v2/id3v2header.cpp \
  taglib-1.12/taglib/mpeg/id3v2/id3v2synchdata.cpp \
  taglib-1.12/taglib/mpeg/id3v2/id3v2footer.cpp \
  taglib-1.12/taglib/mpeg/id3v2/id3v2extendedheader.cpp \
  taglib-1.12/taglib/mpeg/mpegproperties.cpp \
  taglib-1.12/taglib/mpeg/mpegheader.cpp \
  taglib-1.12/taglib/mpeg/mpegfile.cpp \
  taglib-1.12/taglib/mpeg/xingheader.cpp \
  taglib-1.12/taglib/mpeg/id3v1/id3v1tag.cpp \
  taglib-1.12/taglib/mpeg/id3v1/id3v1genres.cpp \
  taglib-1.12/taglib/tagunion.cpp


HEADERS += \
  taglib-1.12/taglib/taglib_config.h \
  taglib-1.12/3rdparty/utf8-cpp/core.h \
  taglib-1.12/3rdparty/utf8-cpp/checked.h \
  taglib-1.12/taglib/toolkit/tlist.h \
  taglib-1.12/taglib/toolkit/tstringlist.h \
  taglib-1.12/taglib/toolkit/tbytevector.h \
  taglib-1.12/taglib/toolkit/tmap.h \
  taglib-1.12/taglib/toolkit/tbytevectorstream.h \
  taglib-1.12/taglib/toolkit/trefcounter.h \
  taglib-1.12/taglib/toolkit/tdebug.h \
  taglib-1.12/taglib/toolkit/tpropertymap.h \
  taglib-1.12/taglib/toolkit/tfile.h \
  taglib-1.12/taglib/toolkit/tzlib.h \
  taglib-1.12/taglib/toolkit/tdebuglistener.h \
  taglib-1.12/taglib/toolkit/taglib.h \
  taglib-1.12/taglib/toolkit/tbytevectorlist.h \
  taglib-1.12/taglib/toolkit/tutils.h \
  taglib-1.12/taglib/toolkit/tstring.h \
  taglib-1.12/taglib/toolkit/tiostream.h \
  taglib-1.12/taglib/toolkit/tfilestream.h \
  taglib-1.12/taglib/xm/xmproperties.h \
  taglib-1.12/taglib/xm/xmfile.h \
  taglib-1.12/taglib/trueaudio/trueaudiofile.h \
  taglib-1.12/taglib/trueaudio/trueaudioproperties.h \
  taglib-1.12/taglib/tag.h \
  taglib-1.12/taglib/tagunion.h \
  taglib-1.12/taglib/ape/apeproperties.h \
  taglib-1.12/taglib/ape/apefooter.h \
  taglib-1.12/taglib/ape/apeitem.h \
  taglib-1.12/taglib/ape/apefile.h \
  taglib-1.12/taglib/ape/apetag.h \
  taglib-1.12/taglib/fileref.h \
  taglib-1.12/taglib/audioproperties.h \
  taglib-1.12/taglib/it/itfile.h \
  taglib-1.12/taglib/it/itproperties.h \
  taglib-1.12/taglib/riff/aiff/aiffproperties.h \
  taglib-1.12/taglib/riff/aiff/aifffile.h \
  taglib-1.12/taglib/riff/rifffile.h \
  taglib-1.12/taglib/riff/wav/infotag.h \
  taglib-1.12/taglib/riff/wav/wavfile.h \
  taglib-1.12/taglib/riff/wav/wavproperties.h \
  taglib-1.12/taglib/riff/riffutils.h \
  taglib-1.12/taglib/flac/flacfile.h \
  taglib-1.12/taglib/flac/flacproperties.h \
  taglib-1.12/taglib/flac/flacpicture.h \
  taglib-1.12/taglib/flac/flacmetadatablock.h \
  taglib-1.12/taglib/flac/flacunknownmetadatablock.h \
  taglib-1.12/taglib/mod/modproperties.h \
  taglib-1.12/taglib/mod/modfileprivate.h \
  taglib-1.12/taglib/mod/modfilebase.h \
  taglib-1.12/taglib/mod/modtag.h \
  taglib-1.12/taglib/mod/modfile.h \
  taglib-1.12/taglib/s3m/s3mproperties.h \
  taglib-1.12/taglib/s3m/s3mfile.h \
  taglib-1.12/taglib/ogg/speex/speexproperties.h \
  taglib-1.12/taglib/ogg/speex/speexfile.h \
  taglib-1.12/taglib/ogg/xiphcomment.h \
  taglib-1.12/taglib/ogg/oggfile.h \
  taglib-1.12/taglib/ogg/flac/oggflacfile.h \
  taglib-1.12/taglib/ogg/opus/opusfile.h \
  taglib-1.12/taglib/ogg/opus/opusproperties.h \
  taglib-1.12/taglib/ogg/oggpage.h \
  taglib-1.12/taglib/ogg/vorbis/vorbisproperties.h \
  taglib-1.12/taglib/ogg/vorbis/vorbisfile.h \
  taglib-1.12/taglib/ogg/oggpageheader.h \
  taglib-1.12/taglib/tagutils.h \
  taglib-1.12/taglib/wavpack/wavpackproperties.h \
  taglib-1.12/taglib/wavpack/wavpackfile.h \
  taglib-1.12/taglib/taglib_export.h \
  taglib-1.12/taglib/mpc/mpcproperties.h \
  taglib-1.12/taglib/mpc/mpcfile.h \
  taglib-1.12/taglib/asf/asftag.h \
  taglib-1.12/taglib/asf/asfpicture.h \
  taglib-1.12/taglib/asf/asfattribute.h \
  taglib-1.12/taglib/asf/asfutils.h \
  taglib-1.12/taglib/asf/asfproperties.h \
  taglib-1.12/taglib/asf/asffile.h \
  taglib-1.12/taglib/mp4/mp4file.h \
  taglib-1.12/taglib/mp4/mp4coverart.h \
  taglib-1.12/taglib/mp4/mp4atom.h \
  taglib-1.12/taglib/mp4/mp4properties.h \
  taglib-1.12/taglib/mp4/mp4item.h \
  taglib-1.12/taglib/mp4/mp4tag.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/popularimeterframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/synchronizedlyricsframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/podcastframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/unknownframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/attachedpictureframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/eventtimingcodesframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/chapterframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/commentsframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/textidentificationframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/uniquefileidentifierframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/ownershipframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/privateframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/tableofcontentsframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/relativevolumeframe.h \
  taglib-1.12/taglib/mpeg/id3v2/frames/urllinkframe.h \
  taglib-1.12/taglib/mpeg/id3v2/id3v2header.h \
  taglib-1.12/taglib/mpeg/id3v2/id3v2frame.h \
  taglib-1.12/taglib/mpeg/id3v2/id3v2tag.h \
  taglib-1.12/taglib/mpeg/id3v2/id3v2framefactory.h \
  taglib-1.12/taglib/mpeg/id3v2/id3v2extendedheader.h \
  taglib-1.12/taglib/mpeg/id3v2/id3v2.h \
  taglib-1.12/taglib/mpeg/id3v2/id3v2footer.h \
  taglib-1.12/taglib/mpeg/id3v2/id3v2synchdata.h \
  taglib-1.12/taglib/mpeg/mpegproperties.h \
  taglib-1.12/taglib/mpeg/xingheader.h \
  taglib-1.12/taglib/mpeg/mpegheader.h \
  taglib-1.12/taglib/mpeg/mpegfile.h \
  taglib-1.12/taglib/mpeg/mpegutils.h \
  taglib-1.12/taglib/mpeg/id3v1/id3v1genres.h \
  taglib-1.12/taglib/mpeg/id3v1/id3v1tag.h

INCLUDEPATH += \
  taglib-1.12/taglib \
  taglib-1.12/taglib/toolkit \
  taglib-1.12/taglib/xm \
  taglib-1.12/taglib/trueaudio \
  taglib-1.12/taglib/ape \
  taglib-1.12/taglib/it \
  taglib-1.12/taglib/riff \
  taglib-1.12/taglib/riff/aiff \
  taglib-1.12/taglib/riff/wav \
  taglib-1.12/taglib/flac \
  taglib-1.12/taglib/mod \
  taglib-1.12/taglib/s3m \
  taglib-1.12/taglib/ogg \
  taglib-1.12/taglib/ogg/speex \
  taglib-1.12/taglib/ogg/flac \
  taglib-1.12/taglib/ogg/opus \
  taglib-1.12/taglib/ogg/vorbis \
  taglib-1.12/taglib/wavpack \
  taglib-1.12/taglib/mpc \
  taglib-1.12/taglib/asf \
  taglib-1.12/taglib/mp4 \
  taglib-1.12/taglib/mpeg \
  taglib-1.12/taglib/mpeg/id3v2 \
  taglib-1.12/taglib/mpeg/id3v2/frames \
  taglib-1.12/taglib/mpeg/id3v1 \
  taglib-1.12/3rdparty

