#include "status.h"

namespace MpdClient {
  Status::Status(const mpd_status *status) {
    updateFromMpdStatus(status);
  }

  void Status::updateFromMpdStatus(const struct mpd_status *status) {
    if (!status) {
      return;
    }

    volume = mpd_status_get_volume(status);
    repeat = mpd_status_get_repeat(status) != 0;
    random = mpd_status_get_random(status) != 0;

    switch (mpd_status_get_single_state(status)) {
    case MPD_SINGLE_ON: single = SingleOn; break;
    case MPD_SINGLE_ONESHOT: single = SingleOneShot; break;
    case MPD_SINGLE_OFF: single = SingleOff; break;
    default: single = SingleUnknown; break;
    }

    switch (mpd_status_get_consume_state(status)) {
    case MPD_CONSUME_ON: consume = ConsumeOn; break;
    case MPD_CONSUME_ONESHOT: consume = ConsumeOneShot; break;
    case MPD_CONSUME_OFF: consume = ConsumeOff; break;
    default: consume = ConsumeUnknown; break;
    }

    queueLength = mpd_status_get_queue_length(status);
    queueVersion = mpd_status_get_queue_version(status);

    switch (mpd_status_get_state(status)) {
    case MPD_STATE_PLAY: state = Play; break;
    case MPD_STATE_PAUSE: state = Pause; break;
    case MPD_STATE_STOP: state = Stop; break;
    default: state = UnknownState; break;
    }

    crossfade = mpd_status_get_crossfade(status);
    mixRampDb = mpd_status_get_mixrampdb(status);
    mixRampDelay = mpd_status_get_mixrampdelay(status);

    songPos = mpd_status_get_song_pos(status);
    songId = mpd_status_get_song_id(status);
    nextSongPos = mpd_status_get_next_song_pos(status);
    nextSongId = mpd_status_get_next_song_id(status);

    elapsedMs = mpd_status_get_elapsed_ms(status);
    totalTime = mpd_status_get_total_time(status);
    bitrate = mpd_status_get_kbit_rate(status);
    updateId = mpd_status_get_update_id(status);

    const char *part = mpd_status_get_partition(status);
    partition = part ? QString::fromUtf8(part) : QString();

    const char *err = mpd_status_get_error(status);
    error = err ? QString::fromUtf8(err) : QString();

    const struct mpd_audio_format *af = mpd_status_get_audio_format(status);
    if (af) {
      audioFormat.sampleRate = af->sample_rate;
      audioFormat.bits = af->bits;
      audioFormat.channels = af->channels;
    }
  }

  QDebug operator<<(QDebug dbg, const Status &s) {
    dbg.nospace() << "MpdClient::Status("
                  << "volume=" << s.volume
                  << ", repeat=" << s.repeat
                  << ", random=" << s.random
                  << ", single=" << s.single
                  << ", consume=" << s.consume
                  << ", state=" << s.state
                  << ", songPos=" << s.songPos
                  << ", songId=" << s.songId
                  << ", nextSongPos=" << s.nextSongPos
                  << ", nextSongId=" << s.nextSongId
                  << ", elapsedMs=" << s.elapsedMs
                  << ", totalTime=" << s.totalTime
                  << ", bitrate=" << s.bitrate
                  << ", crossfade=" << s.crossfade
                  << ", mixRampDb=" << s.mixRampDb
                  << ", mixRampDelay=" << s.mixRampDelay
                  << ", queueLength=" << s.queueLength
                  << ", queueVersion=" << s.queueVersion
                  << ", updateId=" << s.updateId
                  << ", partition=\"" << s.partition
                  << "\", error=\"" << s.error
                  << "\", audioFormat=("
                  << s.audioFormat.sampleRate << "Hz,"
                  << s.audioFormat.bits << "bit,"
                  << s.audioFormat.channels << "ch))";
    return dbg;
  }
}
