#ifndef MPD_CLIENT_STATUS_H
#define MPD_CLIENT_STATUS_H

#include "mpd/client.h"

#include <QString>
#include <QDebug>
#include <QMetaType>

namespace MpdClient {
  class Status {
    Q_GADGET
  public:
    enum State {
      Stop,
      Play,
      Pause,
      UnknownState
    };
    Q_ENUM(State)

    enum SingleState {
      SingleOff,
      SingleOn,
      SingleOneShot,
      SingleUnknown
    };
    Q_ENUM(SingleState)

    enum ConsumeState {
      ConsumeOff,
      ConsumeOn,
      ConsumeOneShot,
      ConsumeUnknown
    };
    Q_ENUM(ConsumeState)

    Status() = default;
    explicit Status(const struct mpd_status *status);

    void updateFromMpdStatus(const struct mpd_status *status);

    int volume = -1;
    bool repeat = false;
    bool random = false;
    SingleState single = SingleUnknown;
    ConsumeState consume = ConsumeUnknown;
    int queueLength = 0;
    int queueVersion = 0;
    State state = UnknownState;
    int crossfade = 0;
    double mixRampDb = 0.0;
    int mixRampDelay = 0;
    int songPos = -1;
    int songId = -1;
    int nextSongPos = -1;
    int nextSongId = -1;
    int elapsedMs = 0;
    int totalTime = 0;
    int bitrate = 0;
    int updateId = 0;
    QString partition;
    QString error;

    struct AudioFormat {
      unsigned int sampleRate = 0;
      unsigned int bits = 0;
      unsigned int channels = 0;
    } audioFormat;
  };

  QDebug operator<<(QDebug dbg, const Status &s);
}

Q_DECLARE_METATYPE(MpdClient::Status)

#endif // STATUS_H
