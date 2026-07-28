#include "replaygain/tags.h"

#include <fileref.h>
#include <flacfile.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mp4item.h>
#include <mp4tag.h>
#include <mpegfile.h>
#include <opusfile.h>
#include <tag.h>
#include <textidentificationframe.h>
#include <tpropertymap.h>
#include <tstringlist.h>
#include <xiphcomment.h>

#include <QFile>
#include <QList>

#include <string>

namespace ReplayGain {
  namespace {
    const char kTrackGain[] = "REPLAYGAIN_TRACK_GAIN";
    const char kTrackPeak[] = "REPLAYGAIN_TRACK_PEAK";
    const char kAlbumGain[] = "REPLAYGAIN_ALBUM_GAIN";
    const char kAlbumPeak[] = "REPLAYGAIN_ALBUM_PEAK";
    const char kR128TrackGain[] = "R128_TRACK_GAIN";
    const char kR128AlbumGain[] = "R128_ALBUM_GAIN";

#ifdef _WIN32
    using NativeName = std::wstring;
    NativeName nativeName(const QString &path) { return path.toStdWString(); }
    const wchar_t *nativePtr(const NativeName &n) { return n.c_str(); }
#else
    using NativeName = QByteArray;
    NativeName nativeName(const QString &path) { return QFile::encodeName(path); }
    const char *nativePtr(const NativeName &n) { return n.constData(); }
#endif

    QString toQString(const TagLib::String &s) {
      return QString::fromStdString(s.to8Bit(true));
    }

    QString lookup(const TagLib::PropertyMap &props, const char *key) {
      const TagLib::String wanted(key);
      const auto direct = props.find(wanted);
      if (direct != props.end() && !direct->second.isEmpty()) {
        return toQString(direct->second.front());
      }
      for (auto it = props.begin(); it != props.end(); ++it) {
        if (it->first.upper() == wanted && !it->second.isEmpty()) {
          return toQString(it->second.front());
        }
      }
      return QString();
    }

    bool parseGainDb(const QString &raw, double *out) {
      QString s = raw.trimmed();
      if (s.endsWith(QLatin1String("dB"), Qt::CaseInsensitive)) {
        s.chop(2);
        s = s.trimmed();
      }
      bool ok = false;
      const double v = s.toDouble(&ok);
      if (!ok || !std::isfinite(v)) {
        return false;
      }
      *out = v;
      return true;
    }

    bool parsePeak(const QString &raw, double *out) {
      bool ok = false;
      const double v = raw.trimmed().toDouble(&ok);
      if (!ok || !std::isfinite(v) || v <= 0.0) {
        return false;
      }
      *out = v;
      return true;
    }

    bool parseR128(const QString &raw, double *out) {
      bool ok = false;
      const int q = raw.trimmed().toInt(&ok);
      if (!ok) {
        return false;
      }
      *out = dbFromR128(q);
      return true;
    }

    QString gainText(double db) {
      return QString::number(db, 'f', 2) + QLatin1String(" dB");
    }

    QString peakText(double peak) {
      return QString::number(peak, 'f', 6);
    }

    TagLib::String tagString(const QString &s) {
      return TagLib::String(s.toStdString(), TagLib::String::UTF8);
    }

    void setXiph(TagLib::Ogg::XiphComment *tag, const char *key, const QString &value) {
      tag->removeFields(key);
      if (!value.isEmpty()) {
        tag->addField(key, tagString(value), true);
      }
    }

    void fillXiph(TagLib::Ogg::XiphComment *tag, const Gain &gain) {
      setXiph(tag, kTrackGain, gain.has_track ? gainText(gain.track_db) : QString());
      setXiph(tag, kTrackPeak, gain.has_track ? peakText(gain.track_peak) : QString());
      setXiph(tag, kAlbumGain, gain.has_album ? gainText(gain.album_db) : QString());
      setXiph(tag, kAlbumPeak, gain.has_album ? peakText(gain.album_peak) : QString());
    }

    TagResult writeOpus(TagLib::Ogg::Opus::File *file, const Gain &gain) {
      auto *tag = file->tag();
      if (!tag) {
        return TagResult::Unsupported;
      }
      setXiph(tag, kTrackGain, QString());
      setXiph(tag, kAlbumGain, QString());
      setXiph(tag, kR128TrackGain,
              gain.has_track ? QString::number(r128FromDb(gain.track_db)) : QString());
      setXiph(tag, kR128AlbumGain,
              gain.has_album ? QString::number(r128FromDb(gain.album_db)) : QString());
      setXiph(tag, kTrackPeak, gain.has_track ? peakText(gain.track_peak) : QString());
      setXiph(tag, kAlbumPeak, gain.has_album ? peakText(gain.album_peak) : QString());
      return file->save() ? TagResult::Ok : TagResult::SaveFailed;
    }

    TagResult writeFlac(TagLib::FLAC::File *file, const Gain &gain) {
      // FileRef::tag() is a union including ID3v1, which refuses REPLAYGAIN_* keys.
      auto *tag = file->xiphComment(true);
      if (!tag) {
        return TagResult::Unsupported;
      }
      fillXiph(tag, gain);
      return file->save() ? TagResult::Ok : TagResult::SaveFailed;
    }

    void setTxxx(TagLib::ID3v2::Tag *tag, const char *key, const QString &value) {
      const TagLib::String wanted = TagLib::String(key).upper();
      QList<TagLib::ID3v2::Frame *> doomed;
      for (auto *frame : tag->frameList("TXXX")) {
        auto *txxx = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame *>(frame);
        if (txxx && txxx->description().upper() == wanted) {
          doomed.append(frame);
        }
      }
      for (auto *frame : doomed) {
        tag->removeFrame(frame);
      }
      if (value.isEmpty()) {
        return;
      }
      auto *frame = new TagLib::ID3v2::UserTextIdentificationFrame(TagLib::String::Latin1);
      frame->setDescription(TagLib::String(QString::fromLatin1(key).toLower().toStdString()));
      frame->setText(tagString(value));
      tag->addFrame(frame);
    }

    TagResult writeMpeg(TagLib::MPEG::File *file, const Gain &gain) {
      auto *tag = file->ID3v2Tag(true);
      if (!tag) {
        return TagResult::Unsupported;
      }
      setTxxx(tag, kTrackGain, gain.has_track ? gainText(gain.track_db) : QString());
      setTxxx(tag, kTrackPeak, gain.has_track ? peakText(gain.track_peak) : QString());
      setTxxx(tag, kAlbumGain, gain.has_album ? gainText(gain.album_db) : QString());
      setTxxx(tag, kAlbumPeak, gain.has_album ? peakText(gain.album_peak) : QString());
      return file->save(TagLib::MPEG::File::ID3v2, TagLib::File::StripNone, TagLib::ID3v2::v4)
                 ? TagResult::Ok
                 : TagResult::SaveFailed;
    }

    void setAtom(TagLib::MP4::Tag *tag, const char *key, const QString &value) {
      const QString upper = QString::fromLatin1(key);
      const TagLib::String upper_atom = TagLib::String(("----:com.apple.iTunes:" + upper).toStdString());
      const TagLib::String lower_atom =
          TagLib::String(("----:com.apple.iTunes:" + upper.toLower()).toStdString());
      tag->removeItem(upper_atom);
      tag->removeItem(lower_atom);
      if (!value.isEmpty()) {
        tag->setItem(lower_atom, TagLib::MP4::Item(TagLib::StringList(tagString(value))));
      }
    }

    TagResult writeMp4(TagLib::MP4::File *file, const Gain &gain) {
      auto *tag = file->tag();
      if (!tag) {
        return TagResult::Unsupported;
      }
      setAtom(tag, kTrackGain, gain.has_track ? gainText(gain.track_db) : QString());
      setAtom(tag, kTrackPeak, gain.has_track ? peakText(gain.track_peak) : QString());
      setAtom(tag, kAlbumGain, gain.has_album ? gainText(gain.album_db) : QString());
      setAtom(tag, kAlbumPeak, gain.has_album ? peakText(gain.album_peak) : QString());
      return file->save() ? TagResult::Ok : TagResult::SaveFailed;
    }

    TagResult writeGeneric(TagLib::File *file, const Gain &gain) {
      TagLib::Tag *tag = file->tag();
      if (!tag) {
        return TagResult::Unsupported;
      }
      TagLib::PropertyMap props = tag->properties();
      for (const char *key : {kTrackGain, kTrackPeak, kAlbumGain, kAlbumPeak}) {
        props.erase(TagLib::String(key));
      }
      if (gain.has_track) {
        props.insert(kTrackGain, TagLib::StringList(tagString(gainText(gain.track_db))));
        props.insert(kTrackPeak, TagLib::StringList(tagString(peakText(gain.track_peak))));
      }
      if (gain.has_album) {
        props.insert(kAlbumGain, TagLib::StringList(tagString(gainText(gain.album_db))));
        props.insert(kAlbumPeak, TagLib::StringList(tagString(peakText(gain.album_peak))));
      }
      const TagLib::PropertyMap rejected = tag->setProperties(props);
      for (const char *key : {kTrackGain, kAlbumGain}) {
        if (rejected.contains(TagLib::String(key))) {
          return TagResult::Unsupported;
        }
      }
      return file->save() ? TagResult::Ok : TagResult::SaveFailed;
    }
  }

  Gain fromProperties(const TagLib::PropertyMap &props, bool opus) {
    Gain g;

    if (opus) {
      g.has_track = parseR128(lookup(props, kR128TrackGain), &g.track_db);
      g.has_album = parseR128(lookup(props, kR128AlbumGain), &g.album_db);
    }
    if (!g.has_track) {
      g.has_track = parseGainDb(lookup(props, kTrackGain), &g.track_db);
    }
    if (!g.has_album) {
      g.has_album = parseGainDb(lookup(props, kAlbumGain), &g.album_db);
    }

    parsePeak(lookup(props, kTrackPeak), &g.track_peak);
    parsePeak(lookup(props, kAlbumPeak), &g.album_peak);

    return g;
  }

  Gain readTags(const QString &path) {
    const NativeName name = nativeName(path);
    TagLib::FileRef f(nativePtr(name), false);
    if (f.isNull() || !f.tag()) {
      return Gain();
    }
    const bool opus = dynamic_cast<TagLib::Ogg::Opus::File *>(f.file()) != nullptr;
    return fromProperties(f.tag()->properties(), opus);
  }

  TagResult writeTags(const QString &path, const Gain &gain) {
    const NativeName name = nativeName(path);
    TagLib::FileRef f(nativePtr(name), false);
    if (f.isNull() || !f.file() || !f.file()->isValid()) {
      return TagResult::OpenFailed;
    }

    if (auto *opus = dynamic_cast<TagLib::Ogg::Opus::File *>(f.file())) {
      return writeOpus(opus, gain);
    }
    if (auto *flac = dynamic_cast<TagLib::FLAC::File *>(f.file())) {
      return writeFlac(flac, gain);
    }
    if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(f.file())) {
      return writeMpeg(mpeg, gain);
    }
    if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(f.file())) {
      return writeMp4(mp4, gain);
    }
    return writeGeneric(f.file(), gain);
  }
}
