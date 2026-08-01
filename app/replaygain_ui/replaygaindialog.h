#ifndef RG_REPLAYGAINDIALOG_H
#define RG_REPLAYGAINDIALOG_H

#include "replaygain/manager.h"
#include "track.h"

#include <QDialog>
#include <QVector>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QTableWidget;
QT_END_NAMESPACE

namespace Config {
  class Global;
}

namespace ReplayGainUi {
  class ReplayGainDialog : public QDialog {
    Q_OBJECT
  public:
    ReplayGainDialog(ReplayGain::Manager &rg, Config::Global &global_c, bool gain_applies,
                     const QString &unavailable_reason, QWidget *parent = nullptr);

    void setPlaylistTracks(const QVector<Track> &tracks);
    void setSelectedTracks(const QVector<Track> &tracks);
    void setLibraryPaths(const QStringList &paths);

  signals:
    void scanLibraryRequested(ReplayGain::Mode mode, bool force);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    QWidget *buildApplyGroup();
    QWidget *buildStorageGroup();
    QWidget *buildScanGroup();

    void loadSettings();
    void applySettings();
    void updateScanButtons();
    void updateStoreLabel();
    void appendRow(const ReplayGain::SliceResult &result);
    void onScanFinished(int analysed, int failed, bool cancelled);

    ReplayGain::Manager &rg;
    Config::Global &global_conf;

    QVector<Track> playlist_tracks;
    QVector<Track> selected_tracks;
    QStringList library_paths;

    QLabel *note_ = nullptr;
    QComboBox *mode_combo_ = nullptr;
    QDoubleSpinBox *preamp_spin_ = nullptr;
    QDoubleSpinBox *fallback_spin_ = nullptr;
    QCheckBox *clip_check_ = nullptr;
    QRadioButton *sidecar_radio_ = nullptr;
    QRadioButton *tags_radio_ = nullptr;
    QLabel *store_label_ = nullptr;
    QComboBox *scan_mode_combo_ = nullptr;
    QCheckBox *force_check_ = nullptr;
    QPushButton *scan_library_ = nullptr;
    QPushButton *scan_playlist_ = nullptr;
    QPushButton *scan_selection_ = nullptr;
    QPushButton *cancel_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QLabel *progress_label_ = nullptr;
    QTableWidget *results_ = nullptr;

    bool updating_ = false;
  };
}

#endif // RG_REPLAYGAINDIALOG_H
