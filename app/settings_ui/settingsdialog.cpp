#include "settings_ui/settingsdialog.h"
#include "playlist_ui/columnsconfig.h"
#include "config/storage.h"

#include <algorithm>

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {
  constexpr int BUFFER_BYTES_PER_KIB = 1024;

  struct LanguageEntry {
    const char *code;
    const char *label;
  };
  // Translations shipped under app/resources/translations/. Empty code = follow system locale.
  const LanguageEntry kLanguages[] = {
    {"",   "System default"},
    {"en", "English"},
    {"ja", "Japanese"},
    {"ru", "Russian"},
    {"sr", "Serbian"},
  };

  const QStringList kColumnFields = {
    "artist", "album", "title", "year", "length", "track_number",
    "path", "url", "filename", "format", "bitrate", "channels", "sample_rate",
  };

  // Online names must match Lyrics::ProviderChain::knownProviders().
  const QStringList kLyricsProviders = {"embedded", "sidecar", "lrclib", "netease", "qq", "lyrics.ovh"};
}

SettingsDialog::SettingsDialog(Config::Global &global_c, Config::Local &local_c, QWidget *parent) :
  QDialog(parent), global_conf(global_c), local_conf(local_c)
{
  setWindowTitle(tr("Settings"));
  resize(560, 480);

  auto *tabs = new QTabWidget(this);
  tabs->addTab(buildGeneralTab(),  tr("General"));
  tabs->addTab(buildLyricsTab(),   tr("Lyrics"));
  tabs->addTab(buildAdvancedTab(), tr("Advanced"));

  button_box = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
    this);
  auto *open_config_dir_btn = button_box->addButton(
    tr("Open config directory"), QDialogButtonBox::ActionRole);
  connect(open_config_dir_btn, &QPushButton::clicked, this, []() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(Config::Storage::configPath()));
  });

  auto *root = new QVBoxLayout(this);
  root->addWidget(tabs);
  root->addWidget(button_box);

  populateLanguages();
  populateColumns();
  populateLyrics();
  populateMprisBlacklist();
  fitColumnsTableHeight();

#ifndef Q_OS_MACOS
  // Minimize-to-tray depends on tray-icon being enabled.
  check_minimize_to_tray->setEnabled(check_tray_icon->isChecked());
  connect(check_tray_icon, &QCheckBox::toggled,
          check_minimize_to_tray, &QCheckBox::setEnabled);

  tray_was_enabled = check_tray_icon->isChecked();
#endif

  connect(button_box, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
  connect(button_box, &QDialogButtonBox::clicked,
          this, &SettingsDialog::onButtonBoxClicked);
}

QWidget *SettingsDialog::buildGeneralTab() {
  auto *page = new QWidget;
  auto *vbox = new QVBoxLayout(page);

  // Playback
  auto *gb_playback = new QGroupBox(tr("Playback"));
  auto *pv = new QVBoxLayout(gb_playback);

  check_stop_when_track_removed = new QCheckBox(
    tr("Stop playback when current track or playlist is removed"));
  check_stop_when_track_removed->setChecked(global_conf.stopWhenTrackRemoved());
  pv->addWidget(check_stop_when_track_removed);

  check_inhibit_sleep = new QCheckBox(tr("Prevent system sleep while playing"));
  check_inhibit_sleep->setChecked(global_conf.inhibitSleepWhilePlaying());
  pv->addWidget(check_inhibit_sleep);

  spin_buffer_kib = new QSpinBox;
  spin_buffer_kib->setRange(16, 4096);
  spin_buffer_kib->setSuffix(" " + tr("KiB"));
  // streamBufferSize() is in bytes; default 131072 = 128 KiB.
  int bytes = global_conf.streamBufferSize();
  if (bytes <= 0) bytes = 128 * BUFFER_BYTES_PER_KIB;
  spin_buffer_kib->setValue(bytes / BUFFER_BYTES_PER_KIB);
  auto *buf_row = new QHBoxLayout;
  buf_row->addWidget(new QLabel(tr("Stream buffer size:")));
  buf_row->addWidget(spin_buffer_kib);
  buf_row->addStretch();
  pv->addLayout(buf_row);

  vbox->addWidget(gb_playback);

  // Interface
  auto *gb_iface = new QGroupBox(tr("Interface"));
  auto *iv = new QVBoxLayout(gb_iface);

#ifndef Q_OS_MACOS
  // On macOS the tray icon is replaced by native surfaces (Control Center, Dock
  // menu, menu bar) and close hides rather than quits, so both are meaningless.
  check_tray_icon = new QCheckBox(tr("Show system tray icon"));
  check_tray_icon->setChecked(global_conf.trayIconEnabled());
  iv->addWidget(check_tray_icon);

  check_minimize_to_tray = new QCheckBox(tr("Close to tray instead of quitting"));
  check_minimize_to_tray->setChecked(global_conf.minimizeToTray());
  iv->addWidget(check_minimize_to_tray);
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
  check_auto_update = new QCheckBox(tr("Check for updates on startup"));
  check_auto_update->setChecked(!global_conf.disableAutoUpdateCheck());
  iv->addWidget(check_auto_update);
#endif

  check_row_height = new QCheckBox(tr("Override theme's playlist row height:"));
  spin_row_height = new QSpinBox;
  spin_row_height->setRange(8, 48);
  spin_row_height->setSuffix(" " + tr("px"));

  const int saved_row_height = global_conf.playlistRowHeight();
  const bool override_row_height = saved_row_height != 0;
  check_row_height->setChecked(override_row_height);
  spin_row_height->setEnabled(override_row_height);
  spin_row_height->setValue(override_row_height ? std::clamp(saved_row_height, 8, 48) : 24);

  connect(check_row_height, &QCheckBox::toggled, spin_row_height, &QSpinBox::setEnabled);

  auto *rh_row = new QHBoxLayout;
  rh_row->addWidget(check_row_height);
  rh_row->addWidget(spin_row_height);
  rh_row->addStretch();
  iv->addLayout(rh_row);

  combo_language = new QComboBox;
  auto *lang_row = new QHBoxLayout;
  lang_row->addWidget(new QLabel(tr("Language:")));
  lang_row->addWidget(combo_language);
  auto *lang_hint = new QLabel(tr("(requires restart)"));
  lang_hint->setStyleSheet("color: gray;");
  lang_row->addWidget(lang_hint);
  lang_row->addStretch();
  iv->addLayout(lang_row);

  vbox->addWidget(gb_iface);

  // Library
  auto *gb_library = new QGroupBox(tr("Library"));
  auto *lv = new QVBoxLayout(gb_library);

  combo_library_filter_scope = new QComboBox;
  combo_library_filter_scope->addItem(tr("All levels (default)"), "all_levels");
  combo_library_filter_scope->addItem(tr("Top level only"),       "top_level_only");
  {
    const QString current = global_conf.libraryFilterScope();
    int idx = 0; // fall back to "all_levels" on empty/unknown
    for (int i = 0; i < combo_library_filter_scope->count(); ++i) {
      if (combo_library_filter_scope->itemData(i).toString() == current) {
        idx = i;
        break;
      }
    }
    combo_library_filter_scope->setCurrentIndex(idx);
  }
  auto *lfs_row = new QHBoxLayout;
  lfs_row->addWidget(new QLabel(tr("Filter scope:")));
  lfs_row->addWidget(combo_library_filter_scope);
  lfs_row->addStretch();
  lv->addLayout(lfs_row);

  vbox->addWidget(gb_library);

  // Playlist columns
  auto *gb_cols = new QGroupBox(tr("Playlist columns"));
  auto *cv = new QVBoxLayout(gb_cols);
  table_columns = new QTableWidget;
  table_columns->setColumnCount(4);
  table_columns->setHorizontalHeaderLabels(
    {tr("Field"), tr("Width %"), tr("Alignment"), tr("Stretch")});
  table_columns->verticalHeader()->setVisible(false);
  auto *cols_header = table_columns->horizontalHeader();
  cols_header->setSectionResizeMode(0, QHeaderView::Stretch);          // Field — take slack
  cols_header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Width %
  cols_header->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Alignment
  cols_header->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Stretch (checkbox)
  table_columns->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_columns->setSelectionMode(QAbstractItemView::SingleSelection);
  table_columns->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  table_columns->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  cv->addWidget(table_columns);

  auto *col_btns = new QHBoxLayout;
  auto *btn_add = new QPushButton(tr("Add"));
  auto *btn_rem = new QPushButton(tr("Remove"));
  auto *btn_up  = new QPushButton(tr("Move up"));
  auto *btn_dn  = new QPushButton(tr("Move down"));
  auto *btn_reset_cols = new QPushButton(tr("Restore defaults"));
  col_btns->addWidget(btn_add);
  col_btns->addWidget(btn_rem);
  col_btns->addWidget(btn_up);
  col_btns->addWidget(btn_dn);
  col_btns->addStretch();
  col_btns->addWidget(btn_reset_cols);
  cv->addLayout(col_btns);
  auto *cols_hint = new QLabel(tr("(requires restart)"));
  cols_hint->setStyleSheet("color: gray;");
  cv->addWidget(cols_hint);

  auto add_column_row = [this](const QString &field, int width_pct,
                               const QString &align, bool stretch) {
    int r = table_columns->rowCount();
    table_columns->insertRow(r);

    auto *field_combo = new QComboBox;
    field_combo->addItems(kColumnFields);
    int idx = kColumnFields.indexOf(field);
    if (idx < 0) {
      field_combo->addItem(field);
      idx = field_combo->count() - 1;
    }
    field_combo->setCurrentIndex(idx);
    table_columns->setCellWidget(r, 0, field_combo);

    auto *width_spin = new QSpinBox;
    width_spin->setRange(0, 100);
    width_spin->setValue(width_pct);
    table_columns->setCellWidget(r, 1, width_spin);

    auto *align_combo = new QComboBox;
    align_combo->addItem(tr("Left"),  "left");
    align_combo->addItem(tr("Right"), "right");
    align_combo->setCurrentIndex(align == "right" ? 1 : 0);
    table_columns->setCellWidget(r, 2, align_combo);

    auto *stretch_item = new QTableWidgetItem;
    stretch_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    stretch_item->setCheckState(stretch ? Qt::Checked : Qt::Unchecked);
    table_columns->setItem(r, 3, stretch_item);
  };

  connect(btn_add, &QPushButton::clicked, this, [this, add_column_row]() {
    add_column_row("artist", 20, "left", false);
    fitColumnsTableHeight();
  });
  connect(btn_rem, &QPushButton::clicked, this, [this]() {
    int r = table_columns->currentRow();
    if (r >= 0) {
      table_columns->removeRow(r);
      fitColumnsTableHeight();
    }
  });
  auto move_row = [this](int delta) {
    int r = table_columns->currentRow();
    int nr = r + delta;
    if (r < 0 || nr < 0 || nr >= table_columns->rowCount()) return;
    // Move by reading values and re-creating, since cell widgets don't survive swap_rows cleanly.
    auto read = [this](int row) {
      auto *f  = qobject_cast<QComboBox*>(table_columns->cellWidget(row, 0));
      auto *w  = qobject_cast<QSpinBox*>(table_columns->cellWidget(row, 1));
      auto *a  = qobject_cast<QComboBox*>(table_columns->cellWidget(row, 2));
      auto *s  = table_columns->item(row, 3);
      return std::make_tuple(
        f ? f->currentText() : QString(),
        w ? w->value() : 0,
        a ? a->currentData().toString() : QString("left"),
        s ? (s->checkState() == Qt::Checked) : false);
    };
    auto a_vals = read(r);
    auto b_vals = read(nr);

    auto write = [this](int row, const std::tuple<QString,int,QString,bool> &v) {
      auto *f  = qobject_cast<QComboBox*>(table_columns->cellWidget(row, 0));
      auto *w  = qobject_cast<QSpinBox*>(table_columns->cellWidget(row, 1));
      auto *a  = qobject_cast<QComboBox*>(table_columns->cellWidget(row, 2));
      auto *s  = table_columns->item(row, 3);
      if (f) {
        int idx = f->findText(std::get<0>(v));
        if (idx < 0) { f->addItem(std::get<0>(v)); idx = f->count() - 1; }
        f->setCurrentIndex(idx);
      }
      if (w) w->setValue(std::get<1>(v));
      if (a) a->setCurrentIndex(std::get<2>(v) == "right" ? 1 : 0);
      if (s) s->setCheckState(std::get<3>(v) ? Qt::Checked : Qt::Unchecked);
    };
    write(r, b_vals);
    write(nr, a_vals);
    table_columns->setCurrentCell(nr, 0);
  };
  connect(btn_up, &QPushButton::clicked, this, [move_row]() { move_row(-1); });
  connect(btn_dn, &QPushButton::clicked, this, [move_row]() { move_row(+1); });
  connect(btn_reset_cols, &QPushButton::clicked, this, [this, add_column_row]() {
    table_columns->setRowCount(0);
    PlaylistUi::ColumnsConfig defaults;
    for (int i = 1; i <= defaults.count(); ++i) {
      const bool right = (defaults.align(i) & Qt::AlignRight) == Qt::AlignRight;
      add_column_row(defaults.field(i),
                     static_cast<int>(defaults.width(i) * 100),
                     right ? "right" : "left",
                     defaults.stretch(i));
    }
    fitColumnsTableHeight();
  });

  vbox->addWidget(gb_cols);
  vbox->addStretch();

  return page;
}

QWidget *SettingsDialog::buildLyricsTab() {
  auto *page = new QWidget;
  auto *vbox = new QVBoxLayout(page);
  vbox->addWidget(new QLabel(
    tr("Provider order (drag to reorder, uncheck to disable):")));

  list_lyrics = new QListWidget;
  list_lyrics->setDragDropMode(QAbstractItemView::InternalMove);
  list_lyrics->setSelectionMode(QAbstractItemView::SingleSelection);
  vbox->addWidget(list_lyrics);

  return page;
}

QWidget *SettingsDialog::buildAdvancedTab() {
  auto *page = new QWidget;
  auto *vbox = new QVBoxLayout(page);

  // Single instance
  auto *si_row = new QHBoxLayout;
  check_single_instance = new QCheckBox(tr("Single instance mode"));
  check_single_instance->setChecked(global_conf.singleInstance());
  si_row->addWidget(check_single_instance);
  auto *si_hint = new QLabel(tr("(requires restart)"));
  si_hint->setStyleSheet("color: gray;");
  si_row->addWidget(si_hint);
  si_row->addStretch();
  vbox->addLayout(si_row);

  // IPC port
  auto *port_row = new QHBoxLayout;
  port_row->addWidget(new QLabel(tr("IPC port:")));
  spin_ipc_port = new QSpinBox;
  spin_ipc_port->setRange(1024, 65535);
  spin_ipc_port->setValue(global_conf.ipcPort() > 0 ? global_conf.ipcPort() : 31341);
  port_row->addWidget(spin_ipc_port);
  auto *port_hint = new QLabel(tr("(requires restart)"));
  port_hint->setStyleSheet("color: gray;");
  port_row->addWidget(port_hint);
  port_row->addStretch();
  vbox->addLayout(port_row);

  // Playback log size
  auto *plog_row = new QHBoxLayout;
  plog_row->addWidget(new QLabel(tr("Playback log size:")));
  spin_playback_log_size = new QSpinBox;
  spin_playback_log_size->setRange(10, 10000);
  spin_playback_log_size->setSuffix(" " + tr("entries"));
  int plog_default = global_conf.playbackLogSize();
  spin_playback_log_size->setValue(plog_default > 0 ? plog_default : 100);
  plog_row->addWidget(spin_playback_log_size);
  plog_row->addStretch();
  vbox->addLayout(plog_row);

#ifdef Q_OS_LINUX
  auto *gb_mpris = new QGroupBox(tr("MPRIS blacklist"));
  auto *mv = new QVBoxLayout(gb_mpris);
  mv->addWidget(new QLabel(
    tr("Ignore MPRIS sender names (e.g. \"wireplumber\"):")));
  list_mpris_blacklist = new QListWidget;
  mv->addWidget(list_mpris_blacklist);
  auto *mpris_btns = new QHBoxLayout;
  auto *mp_add = new QPushButton(tr("Add"));
  auto *mp_rem = new QPushButton(tr("Remove"));
  mpris_btns->addWidget(mp_add);
  mpris_btns->addWidget(mp_rem);
  mpris_btns->addStretch();
  mv->addLayout(mpris_btns);
  vbox->addWidget(gb_mpris);

  connect(mp_add, &QPushButton::clicked, this, [this]() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Add MPRIS sender"),
      tr("Sender name:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !name.trimmed().isEmpty()) {
      list_mpris_blacklist->addItem(name.trimmed());
    }
  });
  connect(mp_rem, &QPushButton::clicked, this, [this]() {
    qDeleteAll(list_mpris_blacklist->selectedItems());
  });
#endif

#ifdef ENABLE_MPD_SUPPORT
  check_mpd_stop_on_close = new QCheckBox(tr("Stop MPD playback when closing mpz"));
  check_mpd_stop_on_close->setChecked(global_conf.mpdStopPlayerOnClose());
  vbox->addWidget(check_mpd_stop_on_close);
#endif

  vbox->addStretch();
  return page;
}

void SettingsDialog::populateLanguages() {
  const QString current = global_conf.language();
  for (const auto &e : kLanguages) {
    combo_language->addItem(QString::fromUtf8(e.label), QString::fromUtf8(e.code));
  }
  for (int i = 0; i < combo_language->count(); ++i) {
    if (combo_language->itemData(i).toString() == current) {
      combo_language->setCurrentIndex(i);
      return;
    }
  }
  // Unknown stored value: prepend it so the user sees what's on disk.
  combo_language->addItem(current, current);
  combo_language->setCurrentIndex(combo_language->count() - 1);
}

void SettingsDialog::populateColumns() {
  auto cfg = global_conf.columnsConfig();
  // ColumnsConfig::field/width/etc. use 1-based indexing.
  for (int i = 1; i <= cfg.count(); ++i) {
    int r = table_columns->rowCount();
    table_columns->insertRow(r);

    auto *field_combo = new QComboBox;
    field_combo->addItems(kColumnFields);
    QString fld = cfg.field(i);
    int idx = kColumnFields.indexOf(fld);
    if (idx < 0) {
      field_combo->addItem(fld);
      idx = field_combo->count() - 1;
    }
    field_combo->setCurrentIndex(idx);
    table_columns->setCellWidget(r, 0, field_combo);

    auto *width_spin = new QSpinBox;
    width_spin->setRange(0, 100);
    width_spin->setValue(static_cast<int>(cfg.width(i) * 100));
    table_columns->setCellWidget(r, 1, width_spin);

    auto *align_combo = new QComboBox;
    align_combo->addItem(tr("Left"),  "left");
    align_combo->addItem(tr("Right"), "right");
    const bool right = (cfg.align(i) & Qt::AlignRight) == Qt::AlignRight;
    align_combo->setCurrentIndex(right ? 1 : 0);
    table_columns->setCellWidget(r, 2, align_combo);

    auto *stretch_item = new QTableWidgetItem;
    stretch_item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    stretch_item->setCheckState(cfg.stretch(i) ? Qt::Checked : Qt::Unchecked);
    table_columns->setItem(r, 3, stretch_item);
  }
}

void SettingsDialog::populateLyrics() {
  const QStringList configured = global_conf.lyricsProviders();
  // Configured ones first (in order), then any known-but-disabled providers as unchecked.
  QStringList all;
  for (const auto &p : configured) {
    if (kLyricsProviders.contains(p) && !all.contains(p)) all << p;
  }
  for (const auto &p : kLyricsProviders) {
    if (!all.contains(p)) all << p;
  }
  for (const auto &p : all) {
    QString label;
    if      (p == "embedded")   label = "Embedded (tags)";
    else if (p == "sidecar")    label = "Sidecar (.lrc, .txt)";
    else if (p == "lrclib")     label = "LRCLIB (online)";
    else if (p == "netease")    label = "NetEase Cloud Music (online)";
    else if (p == "qq")         label = "QQ Music (online)";
    else if (p == "lyrics.ovh") label = "Lyrics.ovh (online)";
    else                        label = p;
    auto *item = new QListWidgetItem(label);
    item->setData(Qt::UserRole, p);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
                   Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    item->setCheckState(configured.contains(p) ? Qt::Checked : Qt::Unchecked);
    list_lyrics->addItem(item);
  }
}

void SettingsDialog::fitColumnsTableHeight() {
  table_columns->resizeRowsToContents();
  int h = table_columns->horizontalHeader()->sizeHint().height();
  for (int r = 0; r < table_columns->rowCount(); ++r) {
    h += table_columns->rowHeight(r);
  }
  h += 2 * table_columns->frameWidth();
  table_columns->setFixedHeight(h + 2);
}

void SettingsDialog::populateMprisBlacklist() {
  if (!list_mpris_blacklist) return;
  for (const auto &name : global_conf.mprisBlacklist()) {
    list_mpris_blacklist->addItem(name);
  }
}

PlaylistUi::ColumnsConfig SettingsDialog::collectColumns() const {
  QVector<double> widths;
  QVector<bool> stretches;
  QVector<QString> fields;
  QVector<Qt::Alignment> aligns;

  for (int r = 0; r < table_columns->rowCount(); ++r) {
    auto *f = qobject_cast<QComboBox*>(table_columns->cellWidget(r, 0));
    auto *w = qobject_cast<QSpinBox*>(table_columns->cellWidget(r, 1));
    auto *a = qobject_cast<QComboBox*>(table_columns->cellWidget(r, 2));
    auto *s = table_columns->item(r, 3);

    fields    << (f ? f->currentText() : QString("title"));
    widths    << (w ? w->value() / 100.0 : 0.0);
    aligns    << ((a && a->currentData().toString() == "right")
                  ? (Qt::AlignVCenter | Qt::AlignRight)
                  : Qt::Alignment(Qt::AlignVCenter));
    stretches << (s && s->checkState() == Qt::Checked);
  }

  PlaylistUi::ColumnsConfig result;
  result.setFields(fields);
  result.setWidths(widths);
  result.setAligns(aligns);
  result.setStretches(stretches);
  result.vaidate();
  return result;
}

QStringList SettingsDialog::collectLyrics() const {
  QStringList result;
  for (int i = 0; i < list_lyrics->count(); ++i) {
    auto *item = list_lyrics->item(i);
    if (item->checkState() == Qt::Checked) {
      result << item->data(Qt::UserRole).toString();
    }
  }
  return result;
}

QStringList SettingsDialog::collectMprisBlacklist() const {
  QStringList result;
  if (!list_mpris_blacklist) return result;
  for (int i = 0; i < list_mpris_blacklist->count(); ++i) {
    result << list_mpris_blacklist->item(i)->text();
  }
  return result;
}

void SettingsDialog::apply() {
  // Playback
  global_conf.saveStopWhenTrackRemoved(check_stop_when_track_removed->isChecked());
  global_conf.saveInhibitSleepWhilePlaying(check_inhibit_sleep->isChecked());
  global_conf.saveStreamBufferSize(spin_buffer_kib->value() * BUFFER_BYTES_PER_KIB);

  // Interface
#ifndef Q_OS_MACOS
  global_conf.saveTrayIconEnabled(check_tray_icon->isChecked());
  global_conf.saveMinimizeToTray(check_minimize_to_tray->isChecked());
#endif
  if (check_auto_update) {
    global_conf.saveDisableAutoUpdateCheck(!check_auto_update->isChecked());
  }
  global_conf.savePlaylistRowHeight(
      check_row_height->isChecked() ? spin_row_height->value() : 0);
  global_conf.saveLanguage(combo_language->currentData().toString());

  // Library
  global_conf.saveLibraryFilterScope(combo_library_filter_scope->currentData().toString());

  // Columns
  global_conf.saveColumnsConfig(collectColumns());

  // Lyrics
  global_conf.saveLyricsProviders(collectLyrics());

  // Advanced
  global_conf.saveSingleInstance(check_single_instance->isChecked());
  global_conf.saveIpcPort(spin_ipc_port->value());
  global_conf.savePlaybackLogSize(spin_playback_log_size->value());
  global_conf.saveMprisBlacklist(collectMprisBlacklist());
#ifdef ENABLE_MPD_SUPPORT
  if (check_mpd_stop_on_close) {
    global_conf.saveMpdStopPlayerOnClose(check_mpd_stop_on_close->isChecked());
  }
#endif

  global_conf.sync();
  local_conf.sync();

#ifndef Q_OS_MACOS
  const bool tray_now = check_tray_icon->isChecked();
  if (tray_now != tray_was_enabled) {
    tray_was_enabled = tray_now;
    emit trayIconToggled();
  }
#endif
}

void SettingsDialog::onButtonBoxClicked(QAbstractButton *btn) {
  if (button_box->buttonRole(btn) == QDialogButtonBox::ApplyRole) {
    apply();
  } else if (button_box->buttonRole(btn) == QDialogButtonBox::AcceptRole) {
    apply();
  }
  // RejectRole: do nothing — accept()/reject() handle close.
}
