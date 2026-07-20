#ifndef RADIOSTATIONDIALOG_H
#define RADIOSTATIONDIALOG_H

#include "radio/station.h"

#include <QDialog>

class QLineEdit;
class QSpinBox;

namespace DirectoryUi {
  class RadioStationDialog : public QDialog {
    Q_OBJECT
  public:
    explicit RadioStationDialog(const Radio::Station &station = {}, QWidget *parent = nullptr);

    Radio::Station station() const;

  public slots:
    void accept() override;

  private slots:
    void browseForPlaylist();

  private:
    Radio::Station _station;
    QLineEdit *name_edit;
    QLineEdit *url_edit;
    QLineEdit *group_edit;
    QLineEdit *description_edit;
    QLineEdit *homepage_edit;
    QLineEdit *logo_edit;
    QLineEdit *codec_edit;
    QSpinBox *bitrate_spin;
  };
}

#endif // RADIOSTATIONDIALOG_H
