#include "mpris.h"

#include "player_adaptor.h" // generated
#include "mediaplayer2_adaptor.h" // generated

#include <QDebug>
#include <QHostInfo>

static const auto MPRIS_OBJECT_PATH = "/org/mpris/MediaPlayer2";
static const auto SERVICE_NAME = "org.mpris.MediaPlayer2.mpz";
static const auto MPRIS_ENTRY = "org.mpris.MediaPlayer2.Player";
static const auto FREEDESKTOP_PATH = "org.freedesktop.DBus.Properties";

Mpris::Mpris(Playback::Controller *pl, QObject *parent) : QObject(parent), player(pl), shuffle(false) {
  register_to_dbus();

  connect(this, &Mpris::play, player->controls().play, &QToolButton::click);
  connect(this, &Mpris::pause, player->controls().pause, &QToolButton::click);
  connect(this, &Mpris::stop, player->controls().stop, &QToolButton::click);
  connect(this, &Mpris::next, player->controls().next, &QToolButton::click);
  connect(this, &Mpris::prev, player->controls().prev, &QToolButton::click);

  connect(player, &Playback::Controller::started, [=](const Track &t) {
    Q_UNUSED(t)
    notify("PlaybackStatus", PlaybackStatus());
    notify("Metadata", Metadata());
  });
  connect(player, &Playback::Controller::paused, [=](const Track &t) {
    Q_UNUSED(t)
    notify("PlaybackStatus", PlaybackStatus());
    notify("Metadata", Metadata());
  });
  connect(player, &Playback::Controller::stopped, [=]() {
    notify("PlaybackStatus", PlaybackStatus());
    notify("Metadata", Metadata());
  });
  connect(player, &Playback::Controller::trackChanged, [=](const Track &t) {
    Q_UNUSED(t)
    notify("Metadata", Metadata());
  });

  connect(player, &Playback::Controller::volumeChanged, [=](int val) {
    Q_UNUSED(val)
    notify("Volume", QString("%1").arg(Volume()));
  });

  connect(player, &Playback::Controller::seeked, [&](int val) {
    emit Seeked(val * 1000000);
  });
}

bool Mpris::CanQuit() const {
  return true;
}

bool Mpris::CanRaise() const{
  return false;
}

bool Mpris::HasTrackList() const{
  return false;
}

QString Mpris::Identity() const {
  return qApp->applicationName() + "@" + QHostInfo::localHostName();
}

QString Mpris::DesktopEntry() const {
  return qApp->applicationName();
}

QStringList Mpris::SupportedUriSchemes() const {
  static QStringList res = QStringList() << "file"
                                         << "http"
                                         << "https";
  return res;
}

QStringList Mpris::SupportedMimeTypes() const {
  static QStringList res = QStringList() << "x-content/audio-player"
                                         << "application/ogg"
                                         << "application/x-ogg"
                                         << "application/x-ogm-audio"
                                         << "audio/flac"
                                         << "audio/ogg"
                                         << "audio/vorbis"
                                         << "audio/aac"
                                         << "audio/mp4"
                                         << "audio/mpeg"
                                         << "audio/mpegurl"
                                         << "audio/vnd.rn-realaudio"
                                         << "audio/x-flac"
                                         << "audio/x-oggflac"
                                         << "audio/x-vorbis"
                                         << "audio/x-vorbis+ogg"
                                         << "audio/x-speex"
                                         << "audio/x-wav"
                                         << "audio/x-wavpack"
                                         << "audio/x-ape"
                                         << "audio/x-mp3"
                                         << "audio/x-mpeg"
                                         << "audio/x-mpegurl"
                                         << "audio/x-ms-wma"
                                         << "audio/x-musepack"
                                         << "audio/x-pn-realaudio"
                                         << "audio/x-scpls"
                                         << "video/x-ms-asf";

  return res;
}

QString Mpris::PlaybackStatus() const {
  switch (player->state()) {
    case Playback::Controller::Stopped:
      return "Stopped";
    case Playback::Controller::Paused:
      return "Paused";
    case Playback::Controller::Playing:
      return "Playing";
  }
  return "Stopped";
}

void Mpris::OpenUri(const QString& uri) {
  Q_UNUSED(uri)
}

bool Mpris::Shuffle() const {
  return shuffle;
}

void Mpris::SetShuffle(bool value) {
  emit shuffleChanged(value);
}

QVariantMap Mpris::Metadata() const {
  QVariantMap h;
  h["mpris:trackid"] = QVariant::fromValue(QDBusObjectPath(QString("/%1").arg(player->currentTrack().uid())));
  h["mpris:length"] = player->currentTrack().duration() * 1000000;
  h["xesam:album"] = player->currentTrack().album();
  h["xesam:title"] = player->currentTrack().title();
  h["xesam:trackNumber"] = player->currentTrack().track_number();
  h["xesam:artist"] = player->currentTrack().artist();

  auto art = player->currentTrack().artCover();
  if (!art.isEmpty()) {
    h["mpris:artUrl"] = QUrl::fromLocalFile(art).toString();
  }
  return h;
}

double Mpris::Volume() const {
  return player->volume() / 100.0;
}

void Mpris::SetVolume(double value) {
  player->setVolume(static_cast<int>(value * 100));
}

qlonglong Mpris::Position() const {
  return player->position() * 1000000;
}

bool Mpris::CanGoNext() const {
  return true;
}

bool Mpris::CanGoPrevious() const {
  return true;
}

bool Mpris::CanPlay() const {
  return true;
}

bool Mpris::CanPause() const {
  return true;
}

bool Mpris::CanSeek() const {
  return true;
}

bool Mpris::CanControl() const {
  return true;
}

double Mpris::Rate() const {
  return 1.0;
}

void Mpris::SetRate(double value) {
  Q_UNUSED(value)
}

double Mpris::MaximumRate() const {
  return 1.0;
}

double Mpris::MinimumRate() const {
  return 1.0;
}

void Mpris::on_shuffleChanged(bool val) {
  shuffle = val;
  notify("Shuffle", val);
}

void Mpris::Raise() {
}

void Mpris::Quit() {
  emit quit();
}

void Mpris::Next() {
  emit next();
}

void Mpris::Previous() {
  emit prev();
}

void Mpris::Pause() {
  emit pause();
}

void Mpris::PlayPause() {
  if (player->isStopped()) {
    emit play();
  } else {
    emit pause();
  }
}

void Mpris::Stop() {
  emit stop();
}

void Mpris::Play() {
  emit play();
}

void Mpris::Seek(qlonglong offset) {
  int by = static_cast<int>(offset / 1000000);
  player->seek(player->position() + by);
}

void Mpris::SetPosition(const QDBusObjectPath &trackId, qlonglong offset) {
  Q_UNUSED(trackId)
  int value = static_cast<int>(offset / 1000000);
  player->seek(value);
  Seek(offset);
}

void Mpris::register_to_dbus() {
  if (!QDBusConnection::sessionBus().isConnected()) {
    qWarning() <<"cannot connect to the dbus session bus";
    return;
  }

  if (!QDBusConnection::sessionBus().registerService(SERVICE_NAME)) {
    qWarning() << "cannot register to the session dbus";
    return;
  }

  if (!QDBusConnection::sessionBus().registerObject(MPRIS_OBJECT_PATH, this)) {
    qWarning() << "cannot register object to the dbus";
    return;
  }

  new PlayerAdaptor(this);
  new MediaPlayer2Adaptor(this);
}

void Mpris::notify(const QString &name, const QVariant &value) {
  QDBusMessage msg = QDBusMessage::createSignal(MPRIS_OBJECT_PATH, FREEDESKTOP_PATH, "PropertiesChanged");
  QVariantMap map;
  map.insert(name, value);
  QVariantList args = QVariantList() << MPRIS_ENTRY << map << QStringList();
  msg.setArguments(args);
  if (!QDBusConnection::sessionBus().send(msg)) {
    qWarning() <<"cannot send message to dbus";
  }
}
