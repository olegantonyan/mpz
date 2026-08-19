#ifndef RG_REPLAYGAINDIALOG_H
#define RG_REPLAYGAINDIALOG_H

#include "replaygain/manager.h"
#include "track.h"
#include "config/global.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVector>

namespace ReplayGainUi {
  class ReplayGainDialog : public QDialog {
    Q_OBJECT
  public:
    ReplayGainDialog(ReplayGain::Manager &rg, Config::Global &global_c, bool gain_applies,
                     const QString &unavailable_reason, QWidget *parent = nullptr);

    void setPlaylistTracks(const QVector<Track> &tracks);
    void setSelectedTracks(const QVector<Track> &tracks);
    void setLibraryPaths(const QStringList &paths);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    enum class Scope { Library, Playlist, Selection };

    QWidget *buildPlaybackTab();
    QWidget *buildScanTab();

    Scope currentScope() const;
    void loadSettings();
    void applySettings();
    void updateScanControls();
    void updateStoreLabel();
    void appendRow(const ReplayGain::SliceResult &result);
    void flushRows();
    void onScanFinished(int analysed, int failed, bool cancelled);

    ReplayGain::Manager &rg;
    Config::Global &global_conf;

    QVector<Track> playlist_tracks;
    QVector<Track> selected_tracks;
    QStringList library_paths;

    QLabel *note_ = nullptr;
    QComboBox *mode_combo_ = nullptr;
    QDoubleSpinBox *preamp_spin_ = nullptr;
    QSpinBox *dynamics_spin_ = nullptr;
    QDoubleSpinBox *fallback_spin_ = nullptr;
    QCheckBox *clip_check_ = nullptr;
    QRadioButton *sidecar_radio_ = nullptr;
    QRadioButton *tags_radio_ = nullptr;
    QLabel *store_label_ = nullptr;
    QCheckBox *force_check_ = nullptr;
    QComboBox *scan_scope_combo_ = nullptr;
    QPushButton *scan_button_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QLabel *progress_label_ = nullptr;
    QTableWidget *results_ = nullptr;

    QVector<ReplayGain::SliceResult> pending_rows;
    bool flush_scheduled = false;
    bool updating_ = false;
  };
}

#endif // RG_REPLAYGAINDIALOG_H
