#ifndef GAPLESS_ENGINE_H
#define GAPLESS_ENGINE_H

#include "playback/gapless/pcmcache.h"
#include "playback/gapless/timeline.h"
#include "playback/mediaplayer.h"
#include "track.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QByteArray>
#include <QElapsedTimer>
#include <QMediaDevices>
#include <QObject>
#include <QTimer>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QAudioSink;
class QIODevice;
QT_END_NAMESPACE

namespace Playback::Gapless {
  class TrackDecoder;

  class Engine : public QObject {
    Q_OBJECT
  public:
    explicit Engine(qint64 cache_budget_bytes, QObject *parent = nullptr);
    ~Engine() override;

    Playback::MediaPlayer::State state() const;
    qint64 positionMs() const;

  public slots:
    void setTrack(const Track &t);
    void clearTrack();
    void play();
    void pause();
    void stop();
    void setPositionMs(qint64 ms);
    void setVolume(int pct);
    void prepareNextTrack(const Track &t);
    void setOutputDevice(QByteArray id);

  signals:
    void positionChanged(qint64 track_relative_ms);
    void stateChanged(Playback::MediaPlayer::State s);
    void error(const QString &message);
    void aboutToFinish();
    void nextRequested();

  private:
    void hardSwitchTo(const Track &t);
    void adoptRegion(const Track &t, const Timeline::Pos &pos);
    void newDecoder();

    void onFirstFormat(const QAudioFormat &format);
    void onPcm(const QUrl &url, const QByteArray &bytes);
    void onDecoderFinished(const QUrl &url, qint64 total_frames);
    void onDecodeError(const QUrl &url, const QString &message);

    void prepareSameFileContinuation(const Track &t);
    void prepareNewFile(const Track &t);
    void onPrebufferFirstFormat(const QAudioFormat &format);
    void onPrebufferPcm(const QUrl &url, const QByteArray &bytes);
    void onPrebufferFinished(const QUrl &url, qint64 total_frames);
    void onPrebufferError(const QUrl &url, const QString &message);
    void appendPreparedSegment();
    qint64 preparedEndFrame() const;
    void performDrainRestart();
    void resetPreparedState();
    void clearPreparedFlags();

    void setupPlayback(const QAudioFormat &format);
    void createSink();
    void destroySink();
    void sinkEnsureStarted();
    void feedSink();

    void onPumpTick();
    void emitPosition();
    void applyBackPressure();
    void maybeCloseCurrentAtFrontier();
    qint64 currentFileNeededEnd() const;
    void protectCache();
    void maybeEmitAboutToFinish();
    void checkSegmentBoundary();
    void checkEof();
    void checkStalled();
    void finishPlayback();

    void catchUpTo(qint64 target_abs);
    void restartDecoderForSeek(qint64 target_abs);
    void maybeFinishCatchUp();

    void setState(Playback::MediaPlayer::State s);
    qint64 audibleAbsFrame() const;
    qint64 inFileFrame(qint64 abs_frame) const;
    QAudioDevice outputDevice() const;
    QAudioDevice findPreferredDevice() const;
    void evaluateAudioDevice();
    void switchSink(const QAudioDevice &device);
    QAudioFormat nearestSupported(const QAudioDevice &device, const QAudioFormat &format) const;

    PcmCache cache;
    qint64 cache_budget = 0;
    Timeline timeline;
    TrackDecoder *decoder = nullptr;
    TrackDecoder *prebuffer_decoder = nullptr;
    QAudioSink *sink = nullptr;
    QIODevice *sink_io = nullptr;
    QAudioFormat sink_format;
    QAudioFormat prepared_format;
    QUrl current_url;
    QUrl prepared_url;
    Track current_track;
    Track prepared_track;
    int current_segment = 0;
    qint64 read_cursor_frame = 0;
    qint64 epoch_start_frame = 0; // abs frame the sink clock is anchored to; re-set on every sink (re)start
    qint64 catchup_target_frame = -1;
    qint64 pending_seek_ms = -1; // applied once the pipeline is up after a forced format/device rebuild
    quint64 boundary_adopt_uid = 0; // uid the next setTrack may adopt seamlessly; 0 = none
    qint64 decoder_total_frames = 0;
    qint64 prepared_begin_frame = 0;
    qint64 prepared_end_frame = -1;
    qint64 prepared_total_frames = 0;
    int volume_pct = 100;
    QByteArray output_device_id;
    QAudioDevice active_device; // device the current sink is running on
    bool preferred_device_missing = false; // configured device absent; running on follow-default fallback
    int device_change_epoch = 0; // cancels stale queued device switches during event bursts
    Playback::MediaPlayer::State current_state = Playback::MediaPlayer::StoppedState;
    bool advance_on_eof = false;
    bool synthetic_playing_on_play = false;
    bool decoder_finished = false;
    bool eof_done = false;
    bool format_restart_done = false;
    bool about_to_finish_emitted = false;
    bool prepared_segment_appended = false;
    bool prepared_append_deferred = false;
    bool prepared_drain_restart = false;
    bool prepared_decoder_finished = false;
    bool drain_restart_scheduled = false;
    QElapsedTimer stall_timer; // re-armed whenever the audible frame advances
    qint64 stall_last_audible = -1;
    qint64 stall_recovery_audible = -1; // audible frame of the last recovery attempt; -1 = none
    bool stall_fallback_done = false;
    QTimer pump_timer;
    QTimer position_timer;
    QMediaDevices media_devices;
    QTimer devices_changed_debounce; // single-shot; coalesces audioOutputsChanged bursts
  };
}

#endif // GAPLESS_ENGINE_H
