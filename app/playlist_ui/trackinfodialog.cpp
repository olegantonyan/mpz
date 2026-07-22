#include "trackinfodialog.h"
#include "ui_trackinfodialog.h"

#include "icons.h"
#include "tageditordialog.h"
#include "config/global.h"
#include "coverart/online/downloader.h"
#include "lyrics/lrcparser.h"
#include "lyrics/providerchain.h"
#include "reveal_in_filemanager.h"

#include <taglib.h>

#define MPZ_TAGLIB_SINCE(major, minor) \
  (TAGLIB_MAJOR_VERSION > (major) || (TAGLIB_MAJOR_VERSION == (major) && TAGLIB_MINOR_VERSION >= (minor)))

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>
#include <tstringlist.h>

#include <aiffproperties.h>
#include <apeproperties.h>
#include <asfproperties.h>
#include <flacproperties.h>
#include <itproperties.h>
#include <modproperties.h>
#include <mp4properties.h>
#include <mpcproperties.h>
#include <mpegheader.h>
#include <mpegproperties.h>
#include <opusproperties.h>
#include <s3mproperties.h>
#include <speexproperties.h>
#include <trueaudioproperties.h>
#include <vorbisproperties.h>
#include <wavproperties.h>
#include <wavpackproperties.h>
#include <xmproperties.h>
#if MPZ_TAGLIB_SINCE(2, 0)
  #include <dsdiffproperties.h>
  #include <dsfproperties.h>
#endif
#if MPZ_TAGLIB_SINCE(2, 1)
  #include <shortenproperties.h>
#endif
#if MPZ_TAGLIB_SINCE(2, 2)
  #include <matroskaproperties.h>
#endif

#include <QDateTime>
#include <QFont>
#include <QLocale>
#include <QMenu>
#include <QPalette>
#include <QResizeEvent>
#include <QTabWidget>
#include <QTableView>
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QPixmap>

namespace {
  constexpr int FullValueRole = Qt::UserRole + 1;
  constexpr int kValueLimit = 160;
  constexpr int kTooltipLimit = 600;
  constexpr int kMetadataShare = 75;
  constexpr int kTabsShare = 45;

  bool has_local_file(const Track &t) {
    return !t.isMpd() && !t.isStream() && !t.path().isEmpty() && QFile::exists(t.path());
  }

  QString flatten(const QString &s) {
    if (!s.contains('\n') && !s.contains('\r') && !s.contains('\t')) {
      return s;
    }
    return s.simplified();
  }

  QString elide(const QString &s, int limit) {
    return s.size() <= limit ? s : s.left(limit) + QChar(0x2026);
  }

  QString qstr(const TagLib::String &s) {
    return QString::fromUtf8(s.toCString(true));
  }

  QString qstr_list(const TagLib::StringList &l) {
    QStringList out;
    for (const auto &s : l) {
      out << qstr(s);
    }
    return out.join("; ");
  }

  QString mpeg_version(TagLib::MPEG::Header::Version v) {
    switch (v) {
      case TagLib::MPEG::Header::Version1: return "MPEG-1";
      case TagLib::MPEG::Header::Version2: return "MPEG-2";
      case TagLib::MPEG::Header::Version2_5: return "MPEG-2.5";
#if MPZ_TAGLIB_SINCE(2, 0)
      case TagLib::MPEG::Header::Version4: return "MPEG-4";
#endif
    }
    return QString();
  }

  QString mpeg_channel_mode(TagLib::MPEG::Header::ChannelMode m) {
    switch (m) {
      case TagLib::MPEG::Header::Stereo: return "Stereo";
      case TagLib::MPEG::Header::JointStereo: return "Joint Stereo";
      case TagLib::MPEG::Header::DualChannel: return "Dual Channel";
      case TagLib::MPEG::Header::SingleChannel: return "Mono";
    }
    return QString();
  }

  QString mp4_codec(TagLib::MP4::Properties::Codec c) {
    switch (c) {
      case TagLib::MP4::Properties::AAC: return "AAC";
      case TagLib::MP4::Properties::ALAC: return "ALAC";
      case TagLib::MP4::Properties::Unknown: break;
    }
    return QString();
  }

  QString asf_codec(TagLib::ASF::Properties::Codec c) {
    switch (c) {
      case TagLib::ASF::Properties::WMA1: return "Windows Media Audio 1";
      case TagLib::ASF::Properties::WMA2: return "Windows Media Audio 2";
      case TagLib::ASF::Properties::WMA9Pro: return "Windows Media Audio 9 Pro";
      case TagLib::ASF::Properties::WMA9Lossless: return "Windows Media Audio 9 Lossless";
      case TagLib::ASF::Properties::Unknown: break;
    }
    return QString();
  }

#if MPZ_TAGLIB_SINCE(2, 0)
  QString mpeg_channel_configuration(TagLib::MPEG::Header::ChannelConfiguration c) {
    switch (c) {
      case TagLib::MPEG::Header::FrontCenter: return "Front Center";
      case TagLib::MPEG::Header::FrontLeftRight: return "Front Left/Right";
      case TagLib::MPEG::Header::FrontCenterLeftRight: return "Front Center + Left/Right";
      case TagLib::MPEG::Header::FrontCenterLeftRightBackCenter: return "Front Center + Left/Right, Back Center";
      case TagLib::MPEG::Header::FrontCenterLeftRightBackLeftRight: return "Front Center + Left/Right, Back Left/Right";
      case TagLib::MPEG::Header::FrontCenterLeftRightBackLeftRightLFE: return "Front Center + Left/Right, Back Left/Right, LFE";
      case TagLib::MPEG::Header::FrontCenterLeftRightSideLeftRightBackLeftRightLFE:
        return "Front Center + Left/Right, Side Left/Right, Back Left/Right, LFE";
      case TagLib::MPEG::Header::Custom: break;
    }
    return QString();
  }
#endif

  QString bool_text(bool v) {
    return v ? "yes" : "no";
  }

  QString hex(const TagLib::ByteVector &v) {
    QString out;
    for (unsigned int i = 0; i < v.size(); ++i) {
      out += QString("%1").arg(static_cast<quint8>(v[i]), 2, 16, QChar('0'));
    }
    return out;
  }

  QString wav_format(int f) {
    switch (f) {
      case 0x0001: return "PCM";
      case 0x0003: return "IEEE Float";
      case 0x0006: return "A-law";
      case 0x0007: return "u-law";
      case 0x0055: return "MP3";
      case 0xFFFE: return "Extensible";
    }
    return QString("0x%1").arg(f, 4, 16, QChar('0'));
  }
}

TrackInfoDialog::TrackInfoDialog(const Track &track, Config::Global &global, std::shared_ptr<Playlist::Playlist> playlist, QWidget *parent) :
  QDialog(parent), ui(new Ui::TrackInfoDialog), _track(track), global_conf(global), _playlist(playlist) {
  ui->setupUi(this);

  base_title = windowTitle();
  setWindowTitle(base_title + ": " + track.formattedTitle());
  setup_view(ui->tableView, &model);
  setup_view(ui->tableViewTags, &model_tags);
  setup_view(ui->tableViewFile, &model_file);
  setup_view(ui->tableViewOther, &model_other);
  for (int i = 0; i < ui->tabWidget->count(); ++i) {
    tab_titles << ui->tabWidget->tabText(i);
  }
  setup_table();
  setup_cover_art();
  setup_lyrics();
  // Splitters only honour absolute sizes, so derive them from the dialog's own
  // width instead of fixed pixel counts. Stretch factors keep the proportions
  // once the user resizes.
  const int metadata_width = width() * kMetadataShare / 100;
  ui->splitter->setSizes({metadata_width, width() - metadata_width});
  ui->splitter->setStretchFactor(0, kMetadataShare);
  ui->splitter->setStretchFactor(1, 100 - kMetadataShare);
  ui->metadataSplitter->setSizes({metadata_width * kTabsShare / 100,
                                  metadata_width * (100 - kTabsShare) / 100});
  ui->metadataSplitter->setStretchFactor(0, kTabsShare);
  ui->metadataSplitter->setStretchFactor(1, 100 - kTabsShare);
  track_path = track.path();
  cover_art_path = track.artCover();
  if (!cover_art_path.isEmpty()) {
    ui->labelCoverArt->setContextMenuPolicy(Qt::CustomContextMenu);
  }
  ui->toolButtonOpenFileManager->setVisible(!track.isMpd());
  ui->toolButtonEditTags->setVisible(!track.isCue() && !track.isMpd() && !track.isStream());
}

TrackInfoDialog::~TrackInfoDialog() {
  delete ui;
}

void TrackInfoDialog::setup_table() {
  add_general_rows();
  add_tags_rows();
  add_file_rows();
  add_other_rows();

  const struct {
    QWidget *page;
    QTableView *view;
    QStandardItemModel *m;
  } tabs[] = {
    { ui->tabGeneral, ui->tableView, &model },
    { ui->tabTags, ui->tableViewTags, &model_tags },
    { ui->tabFile, ui->tableViewFile, &model_file },
    { ui->tabOther, ui->tableViewOther, &model_other },
  };

  // Re-adding only the non-empty pages rather than QTabWidget::setTabVisible,
  // which needs Qt 5.15 (openSUSE Leap 15.3 still ships 5.12).
  QWidget *current = ui->tabWidget->currentWidget();
  while (ui->tabWidget->count() > 0) {
    ui->tabWidget->removeTab(0);
  }
  int i = 0;
  for (const auto &t : tabs) {
    t.view->resizeColumnToContents(0);
    t.view->resizeRowsToContents();
    if (t.m->rowCount() > 0) {
      ui->tabWidget->addTab(t.page, tab_titles.at(i));
    }
    ++i;
  }
  const int restored = current ? ui->tabWidget->indexOf(current) : -1;
  if (restored >= 0) {
    ui->tabWidget->setCurrentIndex(restored);
  }
}

void TrackInfoDialog::add_general_rows() {
  if (!_track.artist().isEmpty()) {
    add_table_row(model, tr("Artist"), _track.artist());
  }
  if (!_track.album().isEmpty()) {
    add_table_row(model, tr("Album"), _track.album());
  }
  if (!_track.title().isEmpty()) {
    add_table_row(model, tr("Title"), _track.title());
  }
  if (_track.year() > 0) {
    add_table_row(model, tr("Year"), QString::number(_track.year()));
  }
  if (!_track.isStream()) {
    add_table_row(model, tr("Track number"), QString::number(_track.track_number()));
    add_table_row(model, tr("Duration"), _track.formattedDuration());
  }
  add_table_row(model, tr("Format"), _track.format());
  if (_track.bitrate() > 0) {
    add_table_row(model, tr("Bitrate"), QString("%1 kbps").arg(_track.bitrate()));
  }
  if (_track.sample_rate() > 0) {
    add_table_row(model, tr("Sample rate"), QString("%1 Hz").arg(_track.sample_rate()));
  }
  if (_track.channels() > 0) {
    add_table_row(model, tr("Channels"), QString::number(_track.channels()));
  }
  if (_track.isStream()) {
    add_table_row(model, tr("Stream url"), _track.url().toDisplayString());
  }
  if (!_track.path().isEmpty()) {
    if (_track.isMpd()) {
      auto url = _track.mpd_server_url();
      if (!url.password().isEmpty()) {
        url.setPassword("***");
      }
      add_table_row(model, tr("File path"), url.toString() + "/" + _track.path());
    } else {
      add_table_row(model, tr("File path"), _track.path());
    }
  }
  if (_track.isCue()) {
    add_table_row(model, tr("CUE start at"), Track::formattedTime(_track.begin()));
  }
}

void TrackInfoDialog::add_tags_rows() {
  if (!has_local_file(_track)) {
    return;
  }
  TagLib::FileRef f(QFile::encodeName(_track.path()).constData(), false);
  if (f.isNull() || !f.tag()) {
    return;
  }
  const TagLib::PropertyMap props = f.tag()->properties();
  for (const auto &it : props) {
    add_table_row(model_tags, qstr(it.first), qstr_list(it.second));
  }
  const TagLib::StringList unsupported = props.unsupportedData();
  if (!unsupported.isEmpty()) {
    add_table_row(model_tags, tr("Unsupported tags"), qstr_list(unsupported));
  }
}

void TrackInfoDialog::add_file_rows() {
  if (!has_local_file(_track)) {
    return;
  }
  QFileInfo fi(_track.path());
  add_table_row(model_file, tr("File name"), fi.fileName());
  add_table_row(model_file, tr("Directory"), fi.absolutePath());
  add_table_row(model_file, tr("Size"), tr("%1 (%2 bytes)")
                                            .arg(QLocale().formattedDataSize(fi.size()),
                                                 QLocale().toString(fi.size())));
  add_table_row(model_file, tr("Modified"), QLocale().toString(fi.lastModified(), QLocale::ShortFormat));
  const QDateTime born = fi.birthTime();
  if (born.isValid()) {
    add_table_row(model_file, tr("Created"), QLocale().toString(born, QLocale::ShortFormat));
  }
  add_table_row(model_file, tr("Read-only"), yes_no(!fi.isWritable()));
  if (fi.isSymLink()) {
    add_table_row(model_file, tr("Symlink target"), fi.symLinkTarget());
  }
}

void TrackInfoDialog::add_other_rows() {
  if (!has_local_file(_track)) {
    return;
  }
  TagLib::FileRef f(QFile::encodeName(_track.path()).constData());
  if (f.isNull() || !f.audioProperties()) {
    return;
  }
  auto *p = f.audioProperties();

  add_table_row(model_other, "Length", Track::formattedTime(p->lengthInMilliseconds()));
  add_table_row(model_other, "Length (ms)", QString::number(p->lengthInMilliseconds()));
  add_table_row(model_other, "Bitrate", QString("%1 kbps").arg(p->bitrate()));
  add_table_row(model_other, "Sample rate", QString("%1 Hz").arg(p->sampleRate()));
  add_table_row(model_other, "Channels", QString::number(p->channels()));

  if (auto *x = dynamic_cast<TagLib::FLAC::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Sample frames", QString::number(x->sampleFrames()));
    add_table_row(model_other, "MD5 signature", hex(x->signature()));
  } else if (auto *x = dynamic_cast<TagLib::MPEG::Properties *>(p)) {
    add_table_row(model_other, "Version", mpeg_version(x->version()));
    add_table_row(model_other, "Layer", QString::number(x->layer()));
    add_table_row(model_other, "Channel mode", mpeg_channel_mode(x->channelMode()));
#if MPZ_TAGLIB_SINCE(2, 0)
    const QString cfg = mpeg_channel_configuration(x->channelConfiguration());
    if (!cfg.isEmpty()) {
      add_table_row(model_other, "Channel configuration", cfg);
    }
    add_table_row(model_other, "ADTS", bool_text(x->isADTS()));
#endif
    add_table_row(model_other, "Copyrighted", bool_text(x->isCopyrighted()));
    add_table_row(model_other, "Original", bool_text(x->isOriginal()));
    add_table_row(model_other, "CRC protected", bool_text(x->protectionEnabled()));
  } else if (auto *x = dynamic_cast<TagLib::MP4::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    const QString codec = mp4_codec(x->codec());
    if (!codec.isEmpty()) {
      add_table_row(model_other, "Codec", codec);
    }
    add_table_row(model_other, "Encrypted", bool_text(x->isEncrypted()));
  } else if (auto *x = dynamic_cast<TagLib::Ogg::Vorbis::Properties *>(p)) {
    add_table_row(model_other, "Vorbis version", QString::number(x->vorbisVersion()));
    add_table_row(model_other, "Bitrate maximum", QString("%1 kbps").arg(x->bitrateMaximum()));
    add_table_row(model_other, "Bitrate nominal", QString("%1 kbps").arg(x->bitrateNominal()));
    add_table_row(model_other, "Bitrate minimum", QString("%1 kbps").arg(x->bitrateMinimum()));
  } else if (auto *x = dynamic_cast<TagLib::Ogg::Opus::Properties *>(p)) {
    add_table_row(model_other, "Opus version", QString::number(x->opusVersion()));
    add_table_row(model_other, "Input sample rate", QString("%1 Hz").arg(x->inputSampleRate()));
#if MPZ_TAGLIB_SINCE(2, 3)
    add_table_row(model_other, "Output gain", QString::number(x->outputGain()));
#endif
  } else if (auto *x = dynamic_cast<TagLib::Ogg::Speex::Properties *>(p)) {
    add_table_row(model_other, "Speex version", QString::number(x->speexVersion()));
    add_table_row(model_other, "Bitrate nominal", QString("%1 kbps").arg(x->bitrateNominal()));
  } else if (auto *x = dynamic_cast<TagLib::RIFF::WAV::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Sample frames", QString::number(x->sampleFrames()));
    add_table_row(model_other, "Codec", wav_format(x->format()));
  } else if (auto *x = dynamic_cast<TagLib::RIFF::AIFF::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Sample frames", QString::number(x->sampleFrames()));
    add_table_row(model_other, "AIFF-C", bool_text(x->isAiffC()));
    if (x->isAiffC()) {
      add_table_row(model_other, "Compression type", qstr(TagLib::String(x->compressionType())));
      add_table_row(model_other, "Compression", qstr(x->compressionName()));
    }
  } else if (auto *x = dynamic_cast<TagLib::APE::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Sample frames", QString::number(x->sampleFrames()));
    add_table_row(model_other, "Version", QString::number(x->version()));
  } else if (auto *x = dynamic_cast<TagLib::WavPack::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Sample frames", QString::number(x->sampleFrames()));
    add_table_row(model_other, "Version", QString::number(x->version()));
    add_table_row(model_other, "Lossless", bool_text(x->isLossless()));
#if MPZ_TAGLIB_SINCE(2, 2)
    add_table_row(model_other, "DSD", bool_text(x->isDsd()));
#endif
  } else if (auto *x = dynamic_cast<TagLib::MPC::Properties *>(p)) {
    add_table_row(model_other, "Musepack version", QString::number(x->mpcVersion()));
    add_table_row(model_other, "Total frames", QString::number(x->totalFrames()));
    add_table_row(model_other, "Track gain", QString::number(x->trackGain()));
    add_table_row(model_other, "Track peak", QString::number(x->trackPeak()));
    add_table_row(model_other, "Album gain", QString::number(x->albumGain()));
    add_table_row(model_other, "Album peak", QString::number(x->albumPeak()));
  } else if (auto *x = dynamic_cast<TagLib::TrueAudio::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Sample frames", QString::number(x->sampleFrames()));
    add_table_row(model_other, "TTA version", QString::number(x->ttaVersion()));
  } else if (auto *x = dynamic_cast<TagLib::ASF::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    const QString codec = asf_codec(x->codec());
    if (!codec.isEmpty()) {
      add_table_row(model_other, "Codec", codec);
    }
    add_table_row(model_other, "Codec name", qstr(x->codecName()));
    add_table_row(model_other, "Codec description", qstr(x->codecDescription()));
    add_table_row(model_other, "Encrypted", bool_text(x->isEncrypted()));
  } else if (auto *x = dynamic_cast<TagLib::Mod::Properties *>(p)) {
    add_table_row(model_other, "Instruments", QString::number(x->instrumentCount()));
  } else if (auto *x = dynamic_cast<TagLib::S3M::Properties *>(p)) {
    add_table_row(model_other, "Stereo", bool_text(x->stereo()));
  } else if (auto *x = dynamic_cast<TagLib::IT::Properties *>(p)) {
    add_table_row(model_other, "Stereo", bool_text(x->stereo()));
  } else if (auto *x = dynamic_cast<TagLib::XM::Properties *>(p)) {
    add_table_row(model_other, "Samples", QString::number(x->sampleCount()));
  }
#if MPZ_TAGLIB_SINCE(2, 0)
  else if (auto *x = dynamic_cast<TagLib::DSF::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Sample count", QString::number(x->sampleCount()));
    add_table_row(model_other, "Format version", QString::number(x->formatVersion()));
    add_table_row(model_other, "Format ID", QString::number(x->formatID()));
    add_table_row(model_other, "Channel type", QString::number(x->channelType()));
    add_table_row(model_other, "Block size per channel", QString::number(x->blockSizePerChannel()));
  } else if (auto *x = dynamic_cast<TagLib::DSDIFF::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Sample count", QString::number(x->sampleCount()));
  }
#endif
#if MPZ_TAGLIB_SINCE(2, 1)
  else if (auto *x = dynamic_cast<TagLib::Shorten::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Shorten version", QString::number(x->shortenVersion()));
    add_table_row(model_other, "File type", QString::number(x->fileType()));
  }
#endif
#if MPZ_TAGLIB_SINCE(2, 2)
  else if (auto *x = dynamic_cast<TagLib::Matroska::Properties *>(p)) {
    add_table_row(model_other, "Bits per sample", QString::number(x->bitsPerSample()));
    add_table_row(model_other, "Doc type", qstr(x->docType()));
    add_table_row(model_other, "Doc type version", QString::number(x->docTypeVersion()));
    add_table_row(model_other, "Codec name", qstr(x->codecName()));
    add_table_row(model_other, "Title", qstr(x->title()));
  }
#endif
}

void TrackInfoDialog::add_table_row(QStandardItemModel &m, const QString &title, const QString &content) {
  const QString display = elide(flatten(content), kValueLimit);
  auto *key = new QStandardItem(title);
  auto *value = new QStandardItem(display);
  // Display is flattened and capped; keep the original so copy/search stay faithful.
  value->setData(content, FullValueRole);
  if (display != content) {
    value->setToolTip(elide(content, kTooltipLimit));
  }
  m.appendRow({key, value});
}

QString TrackInfoDialog::yes_no(bool v) const {
  return v ? tr("yes") : tr("no");
}

QString TrackInfoDialog::value_at(const QTableView *view, const QPoint &pos) const {
  const QModelIndex index = view->indexAt(pos);
  const QVariant full = view->model()->data(index, FullValueRole);
  return full.isValid() ? full.toString() : view->model()->data(index).toString();
}

void TrackInfoDialog::setup_view(QTableView *view, QStandardItemModel *m) {
  view->horizontalHeader()->setVisible(false);
  view->verticalHeader()->setVisible(false);
  view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  view->setModel(m);
  view->setWordWrap(false);
  view->setTextElideMode(Qt::ElideRight);
  view->horizontalHeader()->setStretchLastSection(true);
  setup_context_menu(view);
}

void TrackInfoDialog::setup_context_menu(QTableView *view) {
  view->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(view, &QTableView::customContextMenuRequested, this, [=](const QPoint &pos) {
    if (!view->indexAt(pos).isValid()) {
      return;
    }
    QMenu menu;
    QAction copy(tr("Copy"));
    connect(&copy, &QAction::triggered, this, [=]() {
      qApp->clipboard()->setText(value_at(view, pos));
    });
    QAction search(tr("Search on web"));
    connect(&search, &QAction::triggered, this, [=]() {
      auto text = value_at(view, pos).simplified().left(200);
      auto term = QString("https://duckduckgo.com/?q=%1").arg(QString(QUrl::toPercentEncoding(text)));
      QDesktopServices::openUrl(QUrl(term));
    });
    menu.addAction(&copy);
    menu.addAction(&search);
    menu.exec(view->viewport()->mapToGlobal(pos));
  });
}

void TrackInfoDialog::setup_cover_art() {
  // The label's geometry is only final after the layout runs, which is later
  // than any resizeEvent on the dialog; follow the label itself instead.
  ui->labelCoverArt->installEventFilter(this);
  connect(&CoverArt::Online::Downloader::instance(), &CoverArt::Online::Downloader::coverAvailable,
          this, [this](const QString &artist, const QString &album, const QString &) {
    if (_track.isValid() && _track.artist() == artist && _track.album() == album) {
      render_cover_art();
    }
  });
  render_cover_art();
}

void TrackInfoDialog::render_cover_art() {
  cover_art = QPixmap(_track.artCover());
  if (cover_art.isNull()) {
    // Same muted placeholder the dockable cover widget uses.
    ui->labelCoverArt->setForegroundRole(QPalette::PlaceholderText);
    ui->labelCoverArt->setWordWrap(true);
    QFont f = ui->labelCoverArt->font();
    if (f.pointSizeF() > 0) {
      f.setPointSizeF(f.pointSizeF() * 0.85);
    }
    ui->labelCoverArt->setFont(f);
    ui->labelCoverArt->setText(tr("No cover art"));
    return;
  }
  rescale_cover_art();
}

void TrackInfoDialog::rescale_cover_art() {
  if (cover_art.isNull()) {
    return;
  }
  const QSize target = cover_art.size().scaled(ui->labelCoverArt->size(), Qt::KeepAspectRatio);
  if (target.isEmpty()) {
    return;
  }
  ui->labelCoverArt->setPixmap(cover_art.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

bool TrackInfoDialog::eventFilter(QObject *watched, QEvent *event) {
  if (watched == ui->labelCoverArt && event->type() == QEvent::Resize) {
    rescale_cover_art();
  }
  return QDialog::eventFilter(watched, event);
}

void TrackInfoDialog::on_toolButtonOpenFileManager_clicked() {
  revealInFileManager({ track_path });
}

void TrackInfoDialog::on_toolButtonEditTags_clicked() {
  TagEditorDialog *dlg = new TagEditorDialog({_track}, _playlist);
  dlg->setModal(false);
  connect(dlg, &TagEditorDialog::finished, dlg, &TagEditorDialog::deleteLater);
  // Queued: the playlist reloads tracks in response to saved() (via tracksChanged), refresh must run after.
  connect(dlg, &TagEditorDialog::saved, this, &TrackInfoDialog::refresh_track, Qt::QueuedConnection);
  emit tagEditorOpened(dlg);
  dlg->show();
}

void TrackInfoDialog::refresh_track(const QList<quint64> &uids) {
  if (!_playlist || !uids.contains(_track.uid())) {
    return;
  }
  const Track t = _playlist->trackBy(_track.uid());
  if (t.uid() != _track.uid()) {
    return;
  }
  _track = t;
  model.removeRows(0, model.rowCount());
  model_tags.removeRows(0, model_tags.rowCount());
  model_file.removeRows(0, model_file.rowCount());
  model_other.removeRows(0, model_other.rowCount());
  setup_table();
  setWindowTitle(base_title + ": " + _track.formattedTitle());
}

void TrackInfoDialog::setup_lyrics() {
  if (_track.isStream()) {
    ui->lyricsWidget->setVisible(false);
    return;
  }

  QString embedded = fetch_embedded_lyrics();
  if (!embedded.isEmpty()) {
    render_lyrics("embedded", embedded);
    return;
  }
  QString sidecar = fetch_sidecar_lyrics();
  if (!sidecar.isEmpty()) {
    render_lyrics("sidecar", sidecar);
    return;
  }

  const auto online = Lyrics::ProviderChain::filterKnown(global_conf.lyricsProviders());
  if (!online.isEmpty() && !_track.artist().isEmpty() && !_track.title().isEmpty()) {
    render_lyrics_state(tr("Searching lyrics..."));
    auto *chain = new Lyrics::ProviderChain(this);
    connect(chain, &Lyrics::ProviderChain::found, this, [this, chain](const QString &provider, const QString &lyrics) {
      render_lyrics(Lyrics::ProviderChain::displayName(provider), lyrics);
      chain->deleteLater();
    });
    connect(chain, &Lyrics::ProviderChain::notFound, this, [this, chain]() {
      render_lyrics_state(tr("No lyrics found."));
      chain->deleteLater();
    });
    chain->fetch(online, Lyrics::TrackQuery{_track.artist(), _track.title(), _track.album(),
                                            static_cast<int>(_track.duration() / 1000)});
    return;
  }

  render_lyrics_state(tr("No lyrics found."));
}

QString TrackInfoDialog::fetch_embedded_lyrics() const {
  if (_track.isMpd() || _track.isCue() || _track.path().isEmpty() || !QFile::exists(_track.path())) {
    return QString();
  }
  TagLib::FileRef f(_track.path().toUtf8().constData());
  if (f.isNull() || !f.tag()) {
    return QString();
  }
  auto props = f.tag()->properties();
  auto it = props.find("LYRICS");
  if (it == props.end() || it->second.isEmpty()) {
    return QString();
  }
  return QString::fromUtf8(it->second.front().toCString(true));
}

QString TrackInfoDialog::fetch_sidecar_lyrics() const {
  if (_track.isMpd() || _track.path().isEmpty()) {
    return QString();
  }
  QFileInfo fi(_track.path());
  for (const QString &ext : { QStringLiteral("lrc"), QStringLiteral("txt") }) {
    QString candidate = fi.path() + "/" + fi.completeBaseName() + "." + ext;
    if (!QFile::exists(candidate)) {
      continue;
    }
    QFile f(candidate);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      return QString::fromUtf8(f.readAll());
    }
  }
  return QString();
}

void TrackInfoDialog::render_lyrics(const QString &source, const QString &raw) {
  ui->labelLyricsSource->setText(QString("(%1)").arg(source));
  const QString text = Lyrics::LrcParser::looksLikeLrc(raw)
                         ? Lyrics::LrcParser::stripTimestamps(raw)
                         : raw.trimmed();
  ui->plainTextLyrics->setPlainText(text);
}

void TrackInfoDialog::render_lyrics_state(const QString &message) {
  ui->labelLyricsSource->setText(QString());
  ui->plainTextLyrics->clear();
  ui->plainTextLyrics->setPlaceholderText(message);
}

void TrackInfoDialog::on_labelCoverArt_customContextMenuRequested(const QPoint &pos) {
  QMenu menu;
  QAction copy(tr("Copy to clipboard"));
  copy.setIcon(Icons::get(Icons::Icon::Copy));
  connect(&copy, &QAction::triggered, this, [=]() {
    if (!cover_art.isNull()) {
      QApplication::clipboard()->setPixmap(cover_art);
    }
  });

  QAction show_in_filemanager(tr("Open in external viewer"));
  show_in_filemanager.setIcon(Icons::get(Icons::Icon::FolderReveal));
  connect(&show_in_filemanager, &QAction::triggered, this, [=]() {
    if (!cover_art_path.isEmpty()) {
      QDesktopServices::openUrl(QUrl::fromLocalFile(cover_art_path));
    }
  });
  menu.addAction(&copy);
  menu.addAction(&show_in_filemanager);
  menu.exec(ui->labelCoverArt->mapToGlobal(pos));
}
