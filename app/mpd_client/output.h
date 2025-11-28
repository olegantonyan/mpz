#ifndef MPDOUTPUT_H
#define MPDOUTPUT_H

#include "mpd/client.h"

#include <QObject>

namespace MpdClient {
  class Output {
    Q_GADGET
  public:
    enum State {
      STATE_DISABLED = 0,
      STATE_ENABLED = 1
    };
    Q_ENUM(State)

    Output() = default;
    explicit Output(const struct mpd_output *out);

    void updateFromMpdOutput(const struct mpd_output *out);

    bool isValid() const;
    int id() const;
    QString name() const;
    State state() const;
    bool isEnabled() const;

  private:
    int _id = -1;
    QString _name;
    State _state = STATE_DISABLED;
  };

  QDebug operator<<(QDebug dbg, const Output &o);
}

Q_DECLARE_METATYPE(MpdClient::Output)

#endif // MPDOUTPUT_H
