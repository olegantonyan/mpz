#include "playback_log_ui/playbackloguicontroller.h"
#include "playback_log_ui/playbacklogdialog.h"

namespace PlaybackLogUi {
  Controller::Controller(QObject *parent) : QObject(parent) {
    model = new PlaybackLogUi::Model(100, this);
  }

  void Controller::append(const Track &t) {
    if (t.isStream() && t.streamMeta().isEmpty()) {
      return;
    }
    PlaybackLogUi::Item item(t.uid(), t.shortText());
    if (model->rowCount() > 0 && model->last().text == item.text && model->last().track_uid == item.track_uid) {
      return;
    }
    model->append(item);
  }

  void Controller::show_window() {
    PlaybackLogDialog *dlg = new PlaybackLogDialog(model);
    dlg->setModal(false);
    connect(dlg, &PlaybackLogDialog::finished, dlg, &PlaybackLogDialog::deleteLater);
    connect(dlg, &PlaybackLogDialog::jumpToTrack, this, &Controller::jumpToTrack);
    dlg->show();
  }
}
