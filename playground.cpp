#include <iostream>

#include "taglib/fileref.h"
#include "taglib/tag.h"
#ifdef USE_SYSTEM_TAGLIB
  #include "taglib/mp4file.h"
  #include "taglib/id3v2frame.h"
  #include "taglib/attachedpictureframe.h"
  #include "taglib/id3v2tag.h"
  #include "taglib/mpegfile.h"
  #include "taglib/flacfile.h"
  #include "taglib/flacpicture.h"
#else
  #include "mp4/mp4file.h"
  #include "mpeg/id3v2/id3v2frame.h"
  #include "mpeg/id3v2/frames/attachedpictureframe.h"
  #include "mpeg/id3v2/id3v2tag.h"
  #include "mpeg/mpegfile.h"
  #include "flac/flacfile.h"
  #include "flac/flacpicture.h"
#endif

#include <yaml-cpp/yaml.h>

#include "qhotkey.h"

#include "waitingspinnerwidget.h"

#include <QApplication>

int main(int argc, char* argv[]) {
  std::cout << "Hello" << std::endl;

  QApplication app(argc, argv);

  TagLib::FileRef f("/run/media/oleg/c3996ce0-a379-4403-9d64-7d4c0536463f/music/Asgaard/2001 - Ex Oriente Lux/10. In Articulo Mortis.mp3");
  if(!f.isNull()) {
    if (f.tag()) {
      TagLib::Tag *tag = f.tag();
      auto artist = tag->artist().toCString(true);
      std::cout << artist << std::endl;
    }
  }

  YAML::Emitter emitter;

  QHotkey a;

  WaitingSpinnerWidget b;

  return 0;
}
