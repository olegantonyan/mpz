#ifndef EQ_EQUALIZER_DIALOG_H
#define EQ_EQUALIZER_DIALOG_H

#include "eq/eqprofile.h"
#include "config/local.h"
#include "config/global.h"
#include "playback/playbackcontroller.h"
#include "equalizer_ui/eqcurvewidget.h"

#include <QDialog>
#include <QList>
#include <QVector>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QSlider;
class QTabWidget;
class QTableWidget;

namespace EqualizerUi {
  class EqualizerDialog : public QDialog {
    Q_OBJECT
  public:
    EqualizerDialog(Playback::Controller *player, Config::Local &local_c,
                    Config::Global &global_c, QWidget *parent = nullptr);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    Eq::EqProfile &active();

    void rebuildPresetCombo();
    void loadActiveIntoWidgets();
    void applyCurrent();
    void refreshPreampDisplay();

    QWidget *buildGraphicTab();
    QWidget *buildParametricTab();
    void syncGraphicFromProfile();
    void syncTableFromProfile();
    void rebuildBandsFromTable();
    void addBandRow(const Eq::Band &band);
    bool isGraphicForm(const Eq::EqProfile &p) const;

    void onSaveAs();
    void onDeletePreset();
    void onImport();
    void importFromFile();
    void exportProfile(bool graphic);

    Playback::Controller *player_;
    Config::Local &local_conf;
    Config::Global &global_conf;

    QList<Eq::EqProfile> profiles_;
    int active_index_ = 0;
    bool updating_ = false;
    int display_fs_ = 48000;

    QCheckBox *enable_check_ = nullptr;
    QComboBox *preset_combo_ = nullptr;
    QDoubleSpinBox *preamp_spin_ = nullptr;
    QCheckBox *auto_preamp_check_ = nullptr;
    QLabel *note_ = nullptr;
    CurveWidget *curve_ = nullptr;
    QTabWidget *tabs_ = nullptr;
    QVector<QSlider *> sliders_;
    QVector<QLabel *> slider_labels_;
    QLabel *graphic_hint_ = nullptr;
    QTableWidget *table_ = nullptr;
    QCheckBox *assign_device_check_ = nullptr;
  };
}

#endif // EQ_EQUALIZER_DIALOG_H
