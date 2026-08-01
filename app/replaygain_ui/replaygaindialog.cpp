#include "replaygain_ui/replaygaindialog.h"

#include "config/global.h"
#include "replaygain/tags.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace ReplayGainUi {
  namespace {
    const int kMaxResultRows = 2000;

    QString gainText(double db, bool present) {
      return present ? QString::number(db, 'f', 2) + QStringLiteral(" dB") : QStringLiteral("—");
    }

    QString tagStatus(int tag_result) {
      switch (static_cast<ReplayGain::TagResult>(tag_result)) {
        case ReplayGain::TagResult::Ok: return ReplayGainDialog::tr("tags written");
        case ReplayGain::TagResult::Unsupported: return ReplayGainDialog::tr("tags unsupported");
        case ReplayGain::TagResult::OpenFailed: return ReplayGainDialog::tr("tags: open failed");
        case ReplayGain::TagResult::SaveFailed: return ReplayGainDialog::tr("tags: save failed");
      }
      return QString();
    }
  }

  ReplayGainDialog::ReplayGainDialog(ReplayGain::Manager &r, Config::Global &global_c,
                                     bool gain_applies, const QString &unavailable_reason,
                                     QWidget *parent) :
    QDialog(parent), rg(r), global_conf(global_c) {
    setWindowTitle(tr("ReplayGain"));

    auto *root = new QVBoxLayout(this);

    note_ = new QLabel(unavailable_reason);
    note_->setWordWrap(true);
    note_->setStyleSheet("color: #d35400;");
    note_->setVisible(!gain_applies && !unavailable_reason.isEmpty());
    root->addWidget(note_);

    root->addWidget(buildApplyGroup());
    root->addWidget(buildStorageGroup());
    root->addWidget(buildScanGroup());

    results_ = new QTableWidget(0, 5);
    results_->setHorizontalHeaderLabels(
        {tr("Track"), tr("Track gain"), tr("Peak"), tr("Album gain"), tr("Status")});
    results_->setSelectionBehavior(QAbstractItemView::SelectRows);
    results_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    results_->verticalHeader()->setVisible(false);
    results_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i < 5; i++) {
      results_->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    root->addWidget(results_, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    root->addWidget(buttons);

    connect(&rg, &ReplayGain::Manager::progress, this,
            [this](int done, int total, const QString &path) {
              progress_->setMaximum(total);
              progress_->setValue(done);
              progress_->setVisible(total > 0);
              progress_label_->setText(path.isEmpty() ? QString() : QFileInfo(path).fileName());
            });
    connect(&rg, &ReplayGain::Manager::sliceAnalyzed, this, &ReplayGainDialog::appendRow);
    connect(&rg, &ReplayGain::Manager::scanFinished, this, &ReplayGainDialog::onScanFinished);

    loadSettings();
    updateScanButtons();
    updateStoreLabel();

    const int line = fontMetrics().height();
    resize(sizeHint().expandedTo(QSize(line * 44, line * 36)));
  }

  QWidget *ReplayGainDialog::buildApplyGroup() {
    auto *box = new QGroupBox(tr("Apply"));
    auto *layout = new QVBoxLayout(box);

    auto *mode_row = new QHBoxLayout;
    mode_row->addWidget(new QLabel(tr("Mode:")));
    mode_combo_ = new QComboBox;
    mode_combo_->addItem(tr("Off"), QStringLiteral("off"));
    mode_combo_->addItem(tr("Track gain"), QStringLiteral("track"));
    mode_combo_->addItem(tr("Album gain"), QStringLiteral("album"));
    mode_row->addWidget(mode_combo_);
    mode_row->addStretch();
    layout->addLayout(mode_row);

    auto *preamp_row = new QHBoxLayout;
    preamp_row->addWidget(new QLabel(tr("Preamp:")));
    preamp_spin_ = new QDoubleSpinBox;
    preamp_spin_->setRange(-15.0, 15.0);
    preamp_spin_->setSingleStep(0.5);
    preamp_spin_->setDecimals(1);
    preamp_spin_->setSuffix(" " + tr("dB"));
    preamp_row->addWidget(preamp_spin_);

    preamp_row->addSpacing(12);
    preamp_row->addWidget(new QLabel(tr("Untagged tracks:")));
    fallback_spin_ = new QDoubleSpinBox;
    fallback_spin_->setRange(-15.0, 15.0);
    fallback_spin_->setSingleStep(0.5);
    fallback_spin_->setDecimals(1);
    fallback_spin_->setSuffix(" " + tr("dB"));
    fallback_spin_->setToolTip(tr("Applied to tracks with no ReplayGain data"));
    preamp_row->addWidget(fallback_spin_);
    preamp_row->addStretch();
    layout->addLayout(preamp_row);

    clip_check_ = new QCheckBox(tr("Prevent clipping (use the measured peak)"));
    clip_check_->setToolTip(
        tr("The peak was measured without the equalizer, so with the equalizer boosting "
           "bands the two only compose approximately."));
    layout->addWidget(clip_check_);

    auto apply = [this]() {
      if (!updating_) {
        applySettings();
      }
    };
    connect(mode_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, apply);
    connect(preamp_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, apply);
    connect(fallback_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, apply);
    connect(clip_check_, &QCheckBox::toggled, this, apply);

    return box;
  }

  QWidget *ReplayGainDialog::buildStorageGroup() {
    auto *box = new QGroupBox(tr("Storage"));
    auto *layout = new QVBoxLayout(box);

    sidecar_radio_ = new QRadioButton(tr("Keep audio files unchanged (sidecar database)"));
    tags_radio_ = new QRadioButton(tr("Write ReplayGain tags into the audio files"));
    auto *group = new QButtonGroup(this);
    group->addButton(sidecar_radio_);
    group->addButton(tags_radio_);
    layout->addWidget(sidecar_radio_);
    layout->addWidget(tags_radio_);

    auto *hint = new QLabel(
        tr("Writing tags rewrites each file, so its size and modification time change. "
           "Tracks inside a cue sheet can only be stored in the sidecar."));
    hint->setWordWrap(true);
    hint->setStyleSheet("color: gray;");
    layout->addWidget(hint);

    auto *store_row = new QHBoxLayout;
    store_label_ = new QLabel;
    store_label_->setWordWrap(true);
    store_label_->setStyleSheet("color: gray;");
    store_row->addWidget(store_label_, 1);

    auto *compact = new QPushButton(tr("Compact now"));
    connect(compact, &QPushButton::clicked, this, [this]() {
      rg.store().compact();
      updateStoreLabel();
    });
    store_row->addWidget(compact);
    layout->addLayout(store_row);

    auto apply = [this]() {
      if (!updating_) {
        applySettings();
      }
    };
    connect(sidecar_radio_, &QRadioButton::toggled, this, apply);

    return box;
  }

  QWidget *ReplayGainDialog::buildScanGroup() {
    auto *box = new QGroupBox(tr("Analyse"));
    auto *layout = new QVBoxLayout(box);

    auto *mode_row = new QHBoxLayout;
    mode_row->addWidget(new QLabel(tr("Compute:")));
    scan_mode_combo_ = new QComboBox;
    scan_mode_combo_->addItem(tr("Track gain"), QStringLiteral("track"));
    scan_mode_combo_->addItem(tr("Track and album gain (grouped by folder)"),
                              QStringLiteral("album"));
    scan_mode_combo_->setCurrentIndex(1);
    mode_row->addWidget(scan_mode_combo_);
    mode_row->addStretch();
    layout->addLayout(mode_row);

    force_check_ = new QCheckBox(tr("Re-analyse tracks that already have data"));
    layout->addWidget(force_check_);

    auto scan_mode = [this]() {
      return scan_mode_combo_->currentData().toString() == QLatin1String("album")
                 ? ReplayGain::Mode::Album
                 : ReplayGain::Mode::Track;
    };

    auto *buttons = new QHBoxLayout;
    scan_library_ = new QPushButton(tr("Whole library"));
    connect(scan_library_, &QPushButton::clicked, this, [this, scan_mode]() {
      results_->setRowCount(0);
      emit scanLibraryRequested(scan_mode(), force_check_->isChecked());
      updateScanButtons();
    });
    buttons->addWidget(scan_library_);

    scan_playlist_ = new QPushButton(tr("Current playlist"));
    connect(scan_playlist_, &QPushButton::clicked, this, [this, scan_mode]() {
      results_->setRowCount(0);
      rg.scanTracks(playlist_tracks, scan_mode(), force_check_->isChecked());
      updateScanButtons();
    });
    buttons->addWidget(scan_playlist_);

    scan_selection_ = new QPushButton(tr("Selected tracks"));
    connect(scan_selection_, &QPushButton::clicked, this, [this, scan_mode]() {
      results_->setRowCount(0);
      rg.scanTracks(selected_tracks, scan_mode(), force_check_->isChecked());
      updateScanButtons();
    });
    buttons->addWidget(scan_selection_);

    buttons->addStretch();

    cancel_ = new QPushButton(tr("Cancel"));
    connect(cancel_, &QPushButton::clicked, this, [this]() {
      rg.cancelScan();
      updateScanButtons();
    });
    buttons->addWidget(cancel_);
    layout->addLayout(buttons);

    progress_ = new QProgressBar;
    progress_->setFormat(QStringLiteral("%v / %m"));
    progress_->setVisible(false);
    layout->addWidget(progress_);

    progress_label_ = new QLabel;
    progress_label_->setStyleSheet("color: gray;");
    layout->addWidget(progress_label_);

    return box;
  }

  void ReplayGainDialog::loadSettings() {
    updating_ = true;
    const ReplayGain::Settings s = rg.settings();

    const QString mode = s.mode == ReplayGain::Mode::Album  ? QStringLiteral("album")
                         : s.mode == ReplayGain::Mode::Track ? QStringLiteral("track")
                                                             : QStringLiteral("off");
    mode_combo_->setCurrentIndex(qMax(0, mode_combo_->findData(mode)));
    preamp_spin_->setValue(s.preamp_db);
    fallback_spin_->setValue(s.fallback_db);
    clip_check_->setChecked(s.prevent_clipping);
    sidecar_radio_->setChecked(s.storage == ReplayGain::StorageMode::Sidecar);
    tags_radio_->setChecked(s.storage == ReplayGain::StorageMode::Tags);
    updating_ = false;
  }

  void ReplayGainDialog::applySettings() {
    ReplayGain::Settings s;
    const QString mode = mode_combo_->currentData().toString();
    s.mode = mode == QLatin1String("album")  ? ReplayGain::Mode::Album
             : mode == QLatin1String("track") ? ReplayGain::Mode::Track
                                              : ReplayGain::Mode::Off;
    s.storage = tags_radio_->isChecked() ? ReplayGain::StorageMode::Tags
                                         : ReplayGain::StorageMode::Sidecar;
    s.preamp_db = preamp_spin_->value();
    s.fallback_db = fallback_spin_->value();
    s.prevent_clipping = clip_check_->isChecked();
    rg.setSettings(s);
  }

  void ReplayGainDialog::setPlaylistTracks(const QVector<Track> &tracks) {
    playlist_tracks = tracks;
    updateScanButtons();
  }

  void ReplayGainDialog::setSelectedTracks(const QVector<Track> &tracks) {
    selected_tracks = tracks;
    updateScanButtons();
  }

  void ReplayGainDialog::setLibraryPaths(const QStringList &paths) {
    library_paths = paths;
    updateScanButtons();
  }

  void ReplayGainDialog::updateScanButtons() {
    const bool scanning = rg.isScanning();
    scan_library_->setEnabled(!scanning && !library_paths.isEmpty());
    scan_playlist_->setEnabled(!scanning && !playlist_tracks.isEmpty());
    scan_selection_->setEnabled(!scanning && !selected_tracks.isEmpty());
    cancel_->setEnabled(scanning);
  }

  void ReplayGainDialog::updateStoreLabel() {
    store_label_->setText(tr("%1 — %n entries", "", rg.store().count()).arg(rg.store().filePath()));
  }

  void ReplayGainDialog::appendRow(const ReplayGain::SliceResult &result) {
    if (results_->rowCount() >= kMaxResultRows) {
      results_->removeRow(0);
    }
    const int row = results_->rowCount();
    results_->insertRow(row);

    QString name = QFileInfo(result.path).fileName();
    if (result.begin_ms > 0) {
      name += QString(" @%1s").arg(result.begin_ms / 1000);
    }

    QString status = result.ok ? tr("analysed") : result.error;
    if (result.ok && result.tag_result >= 0) {
      status += QStringLiteral(", ") + tagStatus(result.tag_result);
    }

    results_->setItem(row, 0, new QTableWidgetItem(name));
    results_->setItem(row, 1, new QTableWidgetItem(gainText(result.gain.track_db, result.gain.has_track)));
    results_->setItem(row, 2,
                      new QTableWidgetItem(result.ok ? QString::number(result.gain.track_peak, 'f', 6)
                                                     : QStringLiteral("—")));
    results_->setItem(row, 3, new QTableWidgetItem(gainText(result.gain.album_db, result.gain.has_album)));
    auto *status_item = new QTableWidgetItem(status);
    if (!result.ok) {
      status_item->setForeground(Qt::red);
    }
    results_->setItem(row, 4, status_item);
    results_->scrollToBottom();
  }

  void ReplayGainDialog::onScanFinished(int analysed, int failed, bool cancelled) {
    updateScanButtons();
    updateStoreLabel();
    progress_->setVisible(false);
    if (cancelled) {
      progress_label_->setText(tr("Cancelled after %n track(s)", "", analysed));
    } else {
      progress_label_->setText(tr("Analysed %1, failed %2").arg(analysed).arg(failed));
    }
  }

  void ReplayGainDialog::closeEvent(QCloseEvent *event) {
    global_conf.sync();
    QDialog::closeEvent(event);
  }
}
