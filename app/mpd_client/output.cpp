#include "output.h"

#include <QDebug>

namespace MpdClient {
  Output::Output(const struct mpd_output *out) {
      updateFromMpdOutput(out);
  }

  void Output::updateFromMpdOutput(const struct mpd_output *out) {
    if (!out) {
      _id = -1;
      _name.clear();
      _state = STATE_DISABLED;
      return;
    }

    _id = mpd_output_get_id(out);
    const char *n = mpd_output_get_name(out);
    _name = n ? QString::fromUtf8(n) : QString();

    _state = mpd_output_get_enabled(out) ? STATE_ENABLED : STATE_DISABLED;
  }

  bool Output::isValid() const {
    return _id >= 0;
  }

  int Output::id() const {
    return _id;
  }

  QString Output::name() const {
    return _name;
  }

  Output::State Output::state() const {
    return _state;
  }

  bool Output::isEnabled() const {
    return _state == STATE_ENABLED;
  }

  QDebug operator<<(QDebug dbg, const Output &o) {
    QDebugStateSaver saver(dbg);

    dbg.nospace() << "MpdClient::Output(id=" << o.id()
                  << ", name=\"" << o.name()
                  << "\", enabled=" << o.isEnabled()
                  << ")";

    return dbg;
  }
}
