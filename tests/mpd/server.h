#ifndef MPZ_MPD_TEST_SERVER_H
#define MPZ_MPD_TEST_SERVER_H

#include "mpd_client/entity.h"
#include "mpd_client/output.h"
#include "mpd_client/song.h"
#include "mpd_client/status.h"

#include <flacfile.h>
#include <flacpicture.h>
#include <fileref.h>
#include <tpropertymap.h>

#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QUrl>
#include <QVersionNumber>
#include <QtTest>

#include <cmath>

namespace MpdTest {
  // app/main.cpp holds the real registerMetaTypes(), but main.cpp is in the executable and not in mpz_lib, so test binaries never run it.
  // Without the mpd_idle registration the queued Connection::idleEvent -> Client connection is dropped and no idle-derived signal ever fires.
  inline void registerMetaTypes() {
    qRegisterMetaType<MpdClient::Song>("MpdClient::Song");
    qRegisterMetaType<MpdClient::Entity>("MpdClient::Entity");
    qRegisterMetaType<MpdClient::Status>("MpdClient::Status");
    qRegisterMetaType<MpdClient::Output>("MpdClient::Output");
    qRegisterMetaType<mpd_idle>("mpd_idle");
    qRegisterMetaType<QVector<MpdClient::Entity>>("QVector<MpdClient::Entity>");
    qRegisterMetaType<QVector<MpdClient::Output>>("QVector<MpdClient::Output>");
    qRegisterMetaType<QVector<MpdClient::Song>>("QVector<MpdClient::Song>");
  }

  inline bool writeWav(const QString &path, int seconds) {
    const int rate = 44100;
    const int channels = 2;
    const int frames = rate * seconds;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
      return false;
    }
    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);

    const quint32 data_bytes = static_cast<quint32>(frames) * channels * 2;
    f.write("RIFF");
    out << quint32(36 + data_bytes);
    f.write("WAVE");
    f.write("fmt ");
    out << quint32(16) << quint16(1) << quint16(channels) << quint32(rate)
        << quint32(rate * channels * 2) << quint16(channels * 2) << quint16(16);
    f.write("data");
    out << data_bytes;

    for (int i = 0; i < frames; i++) {
      const qint16 s = static_cast<qint16>(std::lround(0.25 * 32767.0 * std::sin(2.0 * M_PI * 440.0 * i / rate)));
      out << s << s;
    }
    return true;
  }

  inline QByteArray pngBytes() {
    QImage img(4, 4, QImage::Format_RGB32);
    img.fill(Qt::darkCyan);
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return bytes;
  }

  inline bool writeTags(const QString &path, const QString &title, const QString &artist,
                        const QString &album, int track, const QString &date) {
    TagLib::FileRef f(QFile::encodeName(path).constData(), false);
    if (f.isNull() || !f.tag()) {
      return false;
    }
    TagLib::PropertyMap props = f.properties();
    props.replace("TITLE", TagLib::String(title.toUtf8().constData(), TagLib::String::UTF8));
    props.replace("ARTIST", TagLib::String(artist.toUtf8().constData(), TagLib::String::UTF8));
    props.replace("ALBUM", TagLib::String(album.toUtf8().constData(), TagLib::String::UTF8));
    props.replace("TRACKNUMBER", TagLib::String(QString::number(track).toUtf8().constData()));
    props.replace("DATE", TagLib::String(date.toUtf8().constData()));
    f.setProperties(props);
    return f.save();
  }

  inline bool embedPicture(const QString &path, const QByteArray &png) {
    TagLib::FLAC::File f(QFile::encodeName(path).constData());
    if (!f.isValid()) {
      return false;
    }
    auto *pic = new TagLib::FLAC::Picture;
    pic->setType(TagLib::FLAC::Picture::FrontCover);
    pic->setMimeType("image/png");
    pic->setData(TagLib::ByteVector(png.constData(), static_cast<unsigned int>(png.size())));
    f.addPicture(pic);
    return f.save();
  }

  struct Options {
    QString password;
    QStringList permissions{"read", "add", "player", "control", "admin"};
    // 0 picks any free port. Only the default-port case needs a fixed one.
    int preferred_port = 0;
  };

  // Spawns a real mpd against a throwaway config, music tree and port. One server per test binary;
  // ctest may run several binaries at once, so nothing here may touch a fixed path or a fixed port.
  class Server {
  public:
    static constexpr int kSongCount = 6;

    ~Server() { stop(); }

    static QString binary() { return QStandardPaths::findExecutable("mpd"); }
    static bool installed() { return !binary().isEmpty(); }

    bool start(const Options &opts = {}) {
      if (!installed()) {
        fail_reason = "mpd is not installed";
        return false;
      }
      if (!dir.isValid()) {
        fail_reason = "could not create the temp dir";
        return false;
      }
      options = opts;
      port = opts.preferred_port;
      if (!readVersion() || !seedMusic()) {
        return false;
      }
      // mpd only learned the "player" permission in 0.23.
      if (mpd_version < QVersionNumber(0, 23)) {
        options.permissions.removeAll("player");
      }

      const int attempts = options.preferred_port > 0 ? 1 : 3;
      for (int attempt = 0; attempt < attempts; attempt++) {
        if (spawn()) {
          return true;
        }
        stop();
        if (options.preferred_port == 0) {
          port = 0;
        }
      }
      return false;
    }

    void stop() {
      if (!proc) {
        return;
      }
      if (proc->state() != QProcess::NotRunning) {
        proc->terminate();
        if (!proc->waitForFinished(5000)) {
          proc->kill();
          proc->waitForFinished(5000);
        }
      }
      delete proc;
      proc = nullptr;
    }

    // SIGKILL, so mpd gets no chance to shut its listener down cleanly.
    void crash() {
      if (proc && proc->state() != QProcess::NotRunning) {
        proc->kill();
        proc->waitForFinished(5000);
      }
    }

    bool restartSamePort() {
      stop();
      return spawn();
    }

    QUrl url() const { return QUrl(QString("mpd://127.0.0.1:%1").arg(port)); }

    QUrl url(const QString &password) const {
      return QUrl(QString("mpd://:%1@127.0.0.1:%2").arg(password).arg(port));
    }

    int serverPort() const { return port; }
    QString root() const { return dir.path(); }
    QString musicDir() const { return dir.path() + "/music"; }
    QByteArray log() const { return stderr_log; }
    QVersionNumber version() const { return mpd_version; }
    QString failReason() const { return fail_reason; }

    // Raw protocol channel. Independent of the code under test, so it doubles as the assertion oracle and reaches commands Client has no slot for.
    QStringList command(const QString &line) {
      QTcpSocket sock;
      if (!connectAndGreet(sock)) {
        return {};
      }
      return send(sock, line);
    }

    void resetState() {
      QTcpSocket sock;
      if (!connectAndGreet(sock)) {
        return;
      }
      send(sock, "stop");
      send(sock, "clear");
      send(sock, "repeat 0");
      send(sock, "random 0");
      send(sock, "single 0");
      send(sock, "consume 0");
      send(sock, "setvol 50");
      send(sock, "enableoutput 0");
      send(sock, "enableoutput 1");
      for (const QString &line : send(sock, "listplaylists")) {
        if (line.startsWith("playlist: ")) {
          send(sock, "rm \"" + line.mid(10) + "\"");
        }
      }
    }

  private:
    bool readVersion() {
      QProcess p;
      p.start(binary(), {"--version"});
      if (!p.waitForFinished(5000)) {
        fail_reason = "mpd --version did not finish";
        return false;
      }
      const QString head = QString::fromUtf8(p.readAllStandardOutput()).section('\n', 0, 0);
      const auto match = QRegularExpression("(\\d+)\\.(\\d+)\\.(\\d+)").match(head);
      if (!match.hasMatch()) {
        fail_reason = "could not parse: " + head;
        return false;
      }
      mpd_version = QVersionNumber::fromString(match.captured(0));
      return true;
    }

    bool seedMusic() {
      const QString music = musicDir();
      for (const QString &sub : {"wav", "tagged", "covered", "embedded"}) {
        if (!QDir().mkpath(music + "/" + sub)) {
          fail_reason = "could not create " + music + "/" + sub;
          return false;
        }
      }
      if (!QDir().mkpath(dir.path() + "/playlists")) {
        fail_reason = "could not create the playlist dir";
        return false;
      }

      // Long enough that seeking and elapsed have room to be asserted.
      if (!writeWav(music + "/wav/long_a.wav", 30) || !writeWav(music + "/wav/long_b.wav", 30)) {
        fail_reason = "could not write the wav fixtures";
        return false;
      }

      const QString fixtures = QStringLiteral(AUDIO_FIXTURES_DIR);
      struct Copy { const char *from; const char *to; };
      const QVector<Copy> copies{
        {"silence.mp3", "tagged/one.mp3"},
        {"silence.flac", "tagged/two.flac"},
        {"silence.mp3", "covered/withcover.mp3"},
        {"silence.flac", "embedded/embedded.flac"},
      };
      for (const auto &c : copies) {
        if (!QFile::copy(fixtures + "/" + c.from, music + "/" + c.to)) {
          fail_reason = QString("could not copy %1").arg(c.from);
          return false;
        }
        QFile::setPermissions(music + "/" + c.to, QFile::ReadOwner | QFile::WriteOwner);
      }

      if (!writeTags(music + "/tagged/one.mp3", "One", "Tagged Artist", "Tagged Album", 1, "1999") ||
          !writeTags(music + "/tagged/two.flac", "Two", "Tagged Artist", "Tagged Album", 2, "1999")) {
        fail_reason = "could not write tags";
        return false;
      }

      const QByteArray png = pngBytes();
      QFile cover(music + "/covered/cover.png");
      if (!cover.open(QIODevice::WriteOnly) || cover.write(png) != png.size()) {
        fail_reason = "could not write cover.png";
        return false;
      }
      cover.close();

      if (!embedPicture(music + "/embedded/embedded.flac", png)) {
        fail_reason = "could not embed the flac picture";
        return false;
      }
      return true;
    }

    bool pickPort() {
      QTcpServer probe;
      if (!probe.listen(QHostAddress::LocalHost, 0)) {
        fail_reason = "could not bind a probe socket";
        return false;
      }
      port = probe.serverPort();
      probe.close();
      return true;
    }

    bool writeConf() {
      QString conf;
      QTextStream out(&conf);
      out << "music_directory    \"" << musicDir() << "\"\n"
          << "playlist_directory \"" << dir.path() << "/playlists\"\n"
          << "db_file            \"" << dir.path() << "/db\"\n"
          << "state_file         \"" << dir.path() << "/state\"\n"
          << "bind_to_address    \"127.0.0.1\"\n"
          << "port               \"" << port << "\"\n"
          << "auto_update        \"no\"\n"
          << "zeroconf_enabled   \"no\"\n"
          << "max_connections    \"20\"\n"
          << "connection_timeout \"600\"\n"
          << "log_level          \"verbose\"\n";
      if (!options.password.isEmpty()) {
        // Any password at all zeroes the unauthenticated default permissions, which is what makes the permission-denied probe case reachable.
        out << "password \"" << options.password << "@" << options.permissions.join(",") << "\"\n";
      }
      // Two outputs so enabling and disabling one is observable. Only the first carries a mixer: mpd averages volume
      // across mixer-carrying outputs, so a second software mixer would couple the output cases to the volume cases.
      out << "audio_output {\n  type \"null\"\n  name \"mpz test out\"\n  mixer_type \"software\"\n}\n"
          << "audio_output {\n  type \"null\"\n  name \"mpz test out 2\"\n  mixer_type \"none\"\n}\n";

      QFile f(confPath());
      if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail_reason = "could not write mpd.conf";
        return false;
      }
      f.write(conf.toUtf8());
      return true;
    }

    QString confPath() const { return dir.path() + "/mpd.conf"; }

    bool spawn() {
      if (port == 0 && !pickPort()) {
        return false;
      }
      if (!writeConf()) {
        return false;
      }

      stderr_log.clear();
      proc = new QProcess;
      // An unread stderr pipe fills up and blocks the child.
      QObject::connect(proc, &QProcess::readyReadStandardError, proc, [this]() {
        stderr_log.append(proc->readAllStandardError());
      });
      proc->setProgram(binary());
      proc->setArguments({"--stderr", "--no-daemon", confPath()});
      proc->start();
      if (!proc->waitForStarted(5000)) {
        fail_reason = "mpd did not start: " + proc->errorString();
        return false;
      }
      return awaitReady();
    }

    // The OK greeting is not enough: a fresh db_file makes mpd kick off an async scan, so lsinfo legitimately returns nothing right after connecting.
    bool awaitReady() {
      QElapsedTimer clock;
      clock.start();
      bool updated = false;

      while (clock.elapsed() < 30000) {
        if (proc->state() != QProcess::Running) {
          fail_reason = "mpd exited during startup:\n" + QString::fromUtf8(stderr_log);
          return false;
        }
        QTcpSocket sock;
        if (!connectAndGreet(sock)) {
          QTest::qWait(100);
          continue;
        }
        if (!updated) {
          send(sock, "update");
          updated = true;
        }
        bool scanning = false;
        for (const QString &line : send(sock, "status")) {
          if (line.startsWith("updating_db:")) {
            scanning = true;
          }
        }
        if (scanning) {
          QTest::qWait(100);
          continue;
        }
        for (const QString &line : send(sock, "stats")) {
          if (line.startsWith("songs: ")) {
            if (line.mid(7).toInt() == kSongCount) {
              return true;
            }
          }
        }
        QTest::qWait(100);
      }
      fail_reason = "mpd never became ready:\n" + QString::fromUtf8(stderr_log);
      return false;
    }

    bool connectAndGreet(QTcpSocket &sock) {
      sock.connectToHost(QHostAddress::LocalHost, static_cast<quint16>(port));
      if (!sock.waitForConnected(2000) || !sock.canReadLine()) {
        if (!sock.waitForReadyRead(2000)) {
          return false;
        }
      }
      const QString greeting = QString::fromUtf8(sock.readLine()).trimmed();
      if (!greeting.startsWith("OK MPD")) {
        return false;
      }
      if (!options.password.isEmpty()) {
        const QStringList reply = send(sock, "password " + options.password);
        Q_UNUSED(reply)
      }
      return true;
    }

    QStringList send(QTcpSocket &sock, const QString &line) {
      sock.write(line.toUtf8() + "\n");
      if (!sock.waitForBytesWritten(2000)) {
        return {};
      }
      QStringList result;
      QElapsedTimer clock;
      clock.start();
      while (clock.elapsed() < 10000) {
        if (!sock.canReadLine() && !sock.waitForReadyRead(2000)) {
          break;
        }
        while (sock.canReadLine()) {
          const QString reply = QString::fromUtf8(sock.readLine()).trimmed();
          if (reply == "OK" || reply.startsWith("ACK ")) {
            if (reply.startsWith("ACK ")) {
              result << reply;
            }
            return result;
          }
          result << reply;
        }
      }
      return result;
    }

    QTemporaryDir dir;
    QProcess *proc = nullptr;
    int port = 0;
    QByteArray stderr_log;
    QVersionNumber mpd_version;
    Options options;
    QString fail_reason;
  };
}

#endif // MPZ_MPD_TEST_SERVER_H
