#ifndef MPRIS_H
#define MPRIS_H

#include "playback/playbackcontroller.h"

#include <QObject>
#include <QtDBus/QtDBus>

class Mpris : public QObject {
  Q_OBJECT
public:
  explicit Mpris(Playback::Controller *pl, QObject *parent = nullptr);

  //org.mpris.MediaPlayer2 MPRIS 2.0 Root interface
  Q_PROPERTY(bool CanQuit READ CanQuit)
  Q_PROPERTY(bool CanRaise READ CanRaise)
  Q_PROPERTY(bool HasTrackList READ HasTrackList)
  Q_PROPERTY(QString Identity READ Identity)
  Q_PROPERTY(QString DesktopEntry READ DesktopEntry)
  Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes)
  Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes)

  //org.mpris.MediaPlayer2.Player MPRIS 2.0 Player interface
  Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus)
  Q_PROPERTY(bool Shuffle READ Shuffle WRITE SetShuffle)
  Q_PROPERTY(QVariantMap Metadata READ Metadata)
  Q_PROPERTY(double Volume READ Volume WRITE SetVolume)
  Q_PROPERTY(qlonglong Position READ Position)
  Q_PROPERTY(bool CanGoNext READ CanGoNext)
  Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious)
  Q_PROPERTY(bool CanPlay READ CanPlay)
  Q_PROPERTY(bool CanPause READ CanPause)
  Q_PROPERTY(bool CanSeek READ CanSeek)
  Q_PROPERTY(bool CanControl READ CanControl)
  Q_PROPERTY(double Rate READ Rate WRITE SetRate)
  Q_PROPERTY(double MinimumRate READ MinimumRate)
  Q_PROPERTY(double MaximumRate READ MaximumRate)

  // Root Properties
  bool CanQuit() const;
  bool CanRaise() const;
  bool HasTrackList() const;
  QString Identity() const;
  QString DesktopEntry() const;
  QStringList SupportedMimeTypes() const;
  QStringList SupportedUriSchemes() const;

  // Player Properties
  QString PlaybackStatus() const;
  bool Shuffle() const;
  void SetShuffle(bool value);
  QVariantMap Metadata() const;
  double Volume() const;
  void SetVolume(double value);
  qlonglong Position() const;
  bool CanGoNext() const;
  bool CanGoPrevious() const;
  bool CanPlay() const;
  bool CanPause() const;
  bool CanSeek() const;
  bool CanControl() const;
  double Rate() const;
  void SetRate(double value);
  double MaximumRate() const;
  double MinimumRate() const;

signals:
  void play();
  void pause();
  void prev();
  void next();
  void stop();
  void quit();
  void shuffleChanged(bool val);

  void Seeked(qlonglong position);

public slots:
  void on_shuffleChanged(bool val);

private slots:
  void Raise();
  void Quit();
  void Next();
  void Previous();
  void Pause();
  void PlayPause();
  void Stop();
  void Play();
  void Seek(qlonglong offset);
  void SetPosition(const QDBusObjectPath& trackId, qlonglong offset);
  void OpenUri(const QString &uri);

private:
  Playback::Controller *player;
  void register_to_dbus();
  void notify(const QString &name, const QVariant &value);
  bool shuffle;
};

#endif // MPRIS_H
