#include "mainwindow.h"

#include <QApplication>
#include <QDebug>


#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "fileref.h"
#include "tag.h"
#include "tpropertymap.h"

using namespace std;


int main(int argc, char *argv[]) {


  for(int i = 1; i < 2; i++) {


    TagLib::FileRef f("/mnt/storage/music/[2008] OST Death Race/01. A Hard Sport For A Hard Age.mp3");

    if(!f.isNull() && f.tag()) {

      TagLib::Tag *tag = f.tag();

      qDebug() << "-- TAG (basic) --" << endl;
      qDebug() << "title   - \"" << tag->title().toCString()   << "\"";
      qDebug() << "artist  - \"" << tag->artist().toCString()  << "\"";
      qDebug() << "album   - \"" << tag->album().toCString()   << "\"";
      qDebug() << "year    - \"" << tag->year()    << "\"";
      qDebug() << "comment - \"" << tag->comment().toCString() << "\"" ;
      qDebug() << "track   - \"" << tag->track()   << "\"";
      qDebug() << "genre   - \"" << tag->genre().toCString()   << "\"";

      TagLib::PropertyMap tags = f.file()->properties();

      unsigned int longest = 0;
      for(TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
        if (i->first.size() > longest) {
          longest = i->first.size();
        }
      }

      cout << "-- TAG (properties) --" << endl;
      for(TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
        for(TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
          cout << left << std::setw(longest) << i->first << " - " << '"' << *j << '"' << endl;
        }
      }

    }

    if(!f.isNull() && f.audioProperties()) {

      TagLib::AudioProperties *properties = f.audioProperties();

      int seconds = properties->length() % 60;
      int minutes = (properties->length() - seconds) / 60;

      cout << "-- AUDIO --" << endl;
      cout << "bitrate     - " << properties->bitrate() << endl;
      cout << "sample rate - " << properties->sampleRate() << endl;
      cout << "channels    - " << properties->channels() << endl;
      cout << "length      - " << minutes << ":" << setfill('0') << setw(2) << seconds << endl;
    }
  }





















  QApplication a(argc, argv);

  QCoreApplication::setOrganizationName("Oleg Antonyan");
  QCoreApplication::setOrganizationDomain("github.com/olegantonyan/mpz");
  QCoreApplication::setApplicationName("mpz");

  MainWindow w;
  w.show();
  return a.exec();
}
