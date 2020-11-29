#include "playback_log_ui/playbackloguicontroller.h"
#include "playback_log_ui/playbacklogdialog.h"

namespace PlaybackLogUi {
  Controller::Controller(Config::Local &local_c, Config::Global &global_c, QObject *parent) : QObject(parent) {
    int size = 100;
    if (global_c.playbackLogSize() > 0) {
      size = global_c.playbackLogSize();
    }
    model = new PlaybackLogUi::Model(local_c, size, this);
  }

  void Controller::append(const Track &t) {
    if (t.isStream() && t.streamMeta().isEmpty()) {
      return;
    }
    PlaybackLogUi::Item item(t.uid(), t.isStream() ? t.shortText() : t.formattedTitle());
    if (model->rowCount() > 0 && model->last().text == item.text && model->last().track_uid == item.track_uid) {
      return;
    }
    model->append(item);
  }

  void Controller::on_monotonicPlaybackTimeIncrement(int by) {
    model->incrementPlayTime(by);
  }

  void Controller::showWindow() {
    PlaybackLogDialog *dlg = new PlaybackLogDialog(model);
    dlg->setModal(false);
    connect(dlg, &PlaybackLogDialog::finished, dlg, &PlaybackLogDialog::deleteLater);
    connect(dlg, &PlaybackLogDialog::jumpToTrack, this, &Controller::jumpToTrack);
    dlg->show();
  }
}
