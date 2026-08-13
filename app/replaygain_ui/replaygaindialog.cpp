#include "replaygain_ui/replaygaindialog.h"

#include "config/global.h"
#include "icons.h"
#include "replaygain/tags.h"
#include "reveal_in_filemanager.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QTabWidget>
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

    auto *tabs = new QTabWidget;
    tabs->addTab(buildPlaybackTab(), tr("Playback"));
    tabs->addTab(buildScanTab(), tr("Analyse"));
    root->addWidget(tabs, 1);

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
    updateScanControls();
    updateStoreLabel();
    tabs->setCurrentIndex(rg.isScanning() ? 1 : 0);
  }

  QWidget *ReplayGainDialog::buildPlaybackTab() {
    auto *box = new QWidget;
    auto *layout = new QVBoxLayout(box);

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    mode_combo_ = new QComboBox;
    mode_combo_->addItem(tr("Off"), QStringLiteral("off"));
    mode_combo_->addItem(tr("Track gain"), QStringLiteral("track"));
    mode_combo_->addItem(tr("Album gain"), QStringLiteral("album"));
    form->addRow(tr("Mode:"), mode_combo_);

    preamp_spin_ = new QDoubleSpinBox;
    preamp_spin_->setRange(-15.0, 15.0);
    preamp_spin_->setSingleStep(0.5);
    preamp_spin_->setDecimals(1);
    preamp_spin_->setSuffix(" " + tr("dB"));
    form->addRow(tr("Preamp:"), preamp_spin_);

    fallback_spin_ = new QDoubleSpinBox;
    fallback_spin_->setRange(-15.0, 15.0);
    fallback_spin_->setSingleStep(0.5);
    fallback_spin_->setDecimals(1);
    fallback_spin_->setSuffix(" " + tr("dB"));
    fallback_spin_->setToolTip(tr("Applied to tracks with no ReplayGain data"));
    form->addRow(tr("Untagged tracks:"), fallback_spin_);

    clip_check_ = new QCheckBox(tr("Prevent clipping (use the measured peak)"));
    clip_check_->setToolTip(
        tr("The peak was measured without the equalizer, so with the equalizer boosting "
           "bands the two only compose approximately."));
    form->addRow(QString(), clip_check_);

    layout->addLayout(form);
    layout->addStretch();

    connect(mode_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReplayGainDialog::applySettings);
    connect(preamp_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ReplayGainDialog::applySettings);
    connect(fallback_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ReplayGainDialog::applySettings);
    connect(clip_check_, &QCheckBox::toggled, this, &ReplayGainDialog::applySettings);

    return box;
  }

  QWidget *ReplayGainDialog::buildScanTab() {
    auto *box = new QWidget;
    auto *layout = new QVBoxLayout(box);

    sidecar_radio_ = new QRadioButton(tr("Store results in a sidecar database"));
    tags_radio_ = new QRadioButton(tr("Write ReplayGain tags into the audio files"));
    auto *group = new QButtonGroup(this);
    group->addButton(sidecar_radio_);
    group->addButton(tags_radio_);
    connect(sidecar_radio_, &QRadioButton::toggled, this, &ReplayGainDialog::applySettings);
    layout->addWidget(sidecar_radio_);
    layout->addWidget(tags_radio_);

    auto *tags_hint = new QLabel(
        tr("Rewrites every analysed file, so its size and modification time change. "
           "Tracks inside a cue sheet can only be stored in the sidecar."));
    tags_hint->setWordWrap(true);
    tags_hint->setStyleSheet("color: #d35400;");
    tags_hint->setVisible(false);
    connect(tags_radio_, &QRadioButton::toggled, tags_hint, &QWidget::setVisible);
    layout->addWidget(tags_hint);

    auto *store_row = new QHBoxLayout;
    store_label_ = new QLabel;
    store_label_->setWordWrap(true);
    store_label_->setStyleSheet("color: gray;");
    store_row->addWidget(store_label_, 1);

    auto *reveal = new QPushButton;
    reveal->setIcon(Icons::get(Icons::Icon::FolderReveal));
    reveal->setToolTip(tr("Show in file manager"));
    connect(reveal, &QPushButton::clicked, this, [this]() {
      revealInFileManager({rg.store().filePath()});
    });
    store_row->addWidget(reveal);
    layout->addLayout(store_row);

    force_check_ = new QCheckBox(tr("Re-analyse tracks that already have data"));
    layout->addWidget(force_check_);

    auto *scope_row = new QHBoxLayout;
    scope_row->addWidget(new QLabel(tr("Scope:")));
    scan_scope_combo_ = new QComboBox;
    scan_scope_combo_->addItem(tr("Whole library"), static_cast<int>(Scope::Library));
    scan_scope_combo_->addItem(tr("Current playlist"), static_cast<int>(Scope::Playlist));
    scan_scope_combo_->addItem(tr("Selected tracks"), static_cast<int>(Scope::Selection));
    connect(scan_scope_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ReplayGainDialog::updateScanControls);
    scope_row->addWidget(scan_scope_combo_);
    scope_row->addStretch();

    scan_button_ = new QPushButton;
    connect(scan_button_, &QPushButton::clicked, this, [this]() {
      if (rg.isScanning()) {
        rg.cancelScan();
        updateScanControls();
        return;
      }
      results_->setRowCount(0);
      const Scope scope = currentScope();
      if (scope == Scope::Library) {
        emit scanLibraryRequested(force_check_->isChecked());
      } else {
        rg.scanTracks(scope == Scope::Playlist ? playlist_tracks : selected_tracks,
                      force_check_->isChecked());
      }
      updateScanControls();
    });
    scope_row->addWidget(scan_button_);
    layout->addLayout(scope_row);

    progress_ = new QProgressBar;
    progress_->setFormat(QStringLiteral("%v / %m"));
    progress_->setVisible(false);
    layout->addWidget(progress_);

    progress_label_ = new QLabel;
    progress_label_->setStyleSheet("color: gray;");
    layout->addWidget(progress_label_);

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
    layout->addWidget(results_, 1);

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
    if (updating_) {
      return;
    }

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

  ReplayGainDialog::Scope ReplayGainDialog::currentScope() const {
    return static_cast<Scope>(scan_scope_combo_->currentData().toInt());
  }

  void ReplayGainDialog::setPlaylistTracks(const QVector<Track> &tracks) {
    playlist_tracks = tracks;
    updateScanControls();
  }

  void ReplayGainDialog::setSelectedTracks(const QVector<Track> &tracks) {
    selected_tracks = tracks;
    updateScanControls();
  }

  void ReplayGainDialog::setLibraryPaths(const QStringList &paths) {
    library_paths = paths;
    updateScanControls();
  }

  void ReplayGainDialog::updateScanControls() {
    const bool scanning = rg.isScanning();
    const Scope scope = currentScope();
    const bool available = scope == Scope::Library    ? !library_paths.isEmpty()
                           : scope == Scope::Playlist ? !playlist_tracks.isEmpty()
                                                      : !selected_tracks.isEmpty();
    scan_scope_combo_->setEnabled(!scanning);
    scan_button_->setText(scanning ? tr("Cancel") : tr("Analyse"));
    scan_button_->setEnabled(scanning || available);
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
    updateScanControls();
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
