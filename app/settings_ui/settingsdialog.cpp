#include "settings_ui/settingsdialog.h"
#include "playlist_ui/columnsconfig.h"
#include "config/storage.h"
#include "coverart/online/cache.h"
#include "coverart/online/providerchain.h"
#include "lyrics/cache.h"
#include "lyrics/providerchain.h"
#ifdef ENABLE_CRASH_HANDLER
  #include "crash_handler.h"
#endif

#include <algorithm>

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSpinBox>
#include <QSystemTrayIcon>
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
}

SettingsDialog::SettingsDialog(Config::Global &global_c, Config::Local &local_c, QWidget *parent) :
  QDialog(parent), global_conf(global_c), local_conf(local_c)
{
  setWindowTitle(tr("Settings"));
  resize(560, 480);

  auto *tabs = new QTabWidget(this);
  tabs->addTab(buildGeneralTab(),  tr("General"));
  // "&&" so the ampersand renders instead of being eaten as a mnemonic.
  tabs->addTab(buildOnlineTab(),   tr("Online lyrics && covers"));
  tabs->addTab(buildAdvancedTab(), tr("Advanced"));

  button_box = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
    this);

  auto *folder_buttons = new QHBoxLayout;
  auto *open_config_dir_btn = new QPushButton(tr("Open config directory"), this);
  connect(open_config_dir_btn, &QPushButton::clicked, this, []() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(Config::Storage::configPath()));
  });
  folder_buttons->addWidget(open_config_dir_btn);
#ifdef ENABLE_CRASH_HANDLER
  auto *open_crash_log_dir_btn = new QPushButton(tr("Open crash log directory"), this);
  connect(open_crash_log_dir_btn, &QPushButton::clicked, this, []() {
    const QString dir = QFileInfo(QString::fromStdString(mpz::crash_log_path())).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
  });
  folder_buttons->addWidget(open_crash_log_dir_btn);
#endif
  folder_buttons->addStretch();

  auto *root = new QVBoxLayout(this);
  root->addWidget(tabs);
  root->addLayout(folder_buttons);
  root->addWidget(button_box);

  populateLanguages();
  populateColumns();
  populateLyrics();
  populateCovers();
  populateMprisBlacklist();
  fitColumnsTableHeight();

  if (check_minimize_to_tray) {
    check_minimize_to_tray->setEnabled(check_tray_icon->isChecked());
    connect(check_tray_icon, &QCheckBox::toggled,
            check_minimize_to_tray, &QCheckBox::setEnabled);
  }

  tray_was_enabled = check_tray_icon->isChecked();

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

  vbox->addWidget(gb_playback);

  // Interface
  auto *gb_iface = new QGroupBox(tr("Interface"));
  auto *iv = new QVBoxLayout(gb_iface);

#ifdef Q_OS_MACOS
  check_tray_icon = new QCheckBox(tr("Show icon in the menu bar"));
#else
  check_tray_icon = new QCheckBox(tr("Show system tray icon"));
#endif
  check_tray_icon->setChecked(global_conf.trayIconEnabled());
  iv->addWidget(check_tray_icon);

#ifndef Q_OS_MACOS
  if (!QSystemTrayIcon::isSystemTrayAvailable()) {
    auto *tray_warning = new QLabel(
      tr("No system tray detected. On GNOME, install the "
         "\"AppIndicator and KStatusNotifierItem Support\" extension "
         "for the tray icon to appear."));
    tray_warning->setWordWrap(true);
    tray_warning->setStyleSheet("color: #d35400;");
    iv->addWidget(tray_warning);
  }

  check_minimize_to_tray = new QCheckBox(tr("Close to tray instead of quitting"));
  check_minimize_to_tray->setChecked(global_conf.minimizeToTray());
  iv->addWidget(check_minimize_to_tray);
#endif

#if defined(ENABLE_UPDATE_CHECK)
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

QWidget *SettingsDialog::buildOnlineTab() {
  auto *page = new QWidget;
  auto *vbox = new QVBoxLayout(page);

  auto *hint = new QLabel(
    tr("Embedded tags and files next to the music are always used first. "
       "These are the online sources tried when nothing is found locally."));
  hint->setWordWrap(true);
  hint->setForegroundRole(QPalette::PlaceholderText);
  vbox->addWidget(hint);

  auto *gb_lyrics = new QGroupBox(tr("Lyrics"));
  auto *lv = new QVBoxLayout(gb_lyrics);
  lv->addWidget(new QLabel(tr("Drag to reorder, uncheck to disable:")));
  list_lyrics = new QListWidget;
  list_lyrics->setDragDropMode(QAbstractItemView::InternalMove);
  list_lyrics->setSelectionMode(QAbstractItemView::SingleSelection);
  lv->addWidget(list_lyrics);
  lv->addLayout(buildCacheButtons(tr("Open lyrics folder"), tr("Clear downloaded lyrics"),
                                  tr("Delete all lyrics downloaded from online providers?"),
                                  []() { return Lyrics::Cache::instance().dir(); },
                                  []() { return Lyrics::Cache::instance().clear(); }));
  vbox->addWidget(gb_lyrics);

  auto *gb_covers = new QGroupBox(tr("Album covers"));
  auto *cv = new QVBoxLayout(gb_covers);
  cv->addWidget(new QLabel(tr("Download missing covers from (drag to reorder, uncheck to disable):")));
  list_covers = new QListWidget;
  list_covers->setDragDropMode(QAbstractItemView::InternalMove);
  list_covers->setSelectionMode(QAbstractItemView::SingleSelection);
  cv->addWidget(list_covers);
  cv->addLayout(buildCacheButtons(tr("Open covers folder"), tr("Clear downloaded covers"),
                                  tr("Delete all covers downloaded from online providers?"),
                                  []() { return CoverArt::Online::Cache::instance().dir(); },
                                  []() { return CoverArt::Online::Cache::instance().clear(); }));
  vbox->addWidget(gb_covers);

  return page;
}

QLayout *SettingsDialog::buildCacheButtons(const QString &open_text, const QString &clear_text,
                                           const QString &confirm_text,
                                           std::function<QString()> dir,
                                           std::function<int()> clear) {
  auto *row = new QHBoxLayout;
  auto *open_btn = new QPushButton(open_text);
  auto *clear_btn = new QPushButton(clear_text);
  row->addWidget(open_btn);
  row->addWidget(clear_btn);
  row->addStretch();

  connect(open_btn, &QPushButton::clicked, this, [dir]() {
    // The cache constructor creates the directory, so this works before the
    // first download too.
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir()));
  });
  connect(clear_btn, &QPushButton::clicked, this, [this, clear, clear_text, confirm_text]() {
    if (QMessageBox::question(this, clear_text, confirm_text) != QMessageBox::Yes) {
      return;
    }
    QMessageBox::information(this, clear_text, tr("Removed %n file(s).", nullptr, clear()));
  });
  return row;
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

  // Stream buffer size
  auto *buf_row = new QHBoxLayout;
  buf_row->addWidget(new QLabel(tr("Stream buffer size:")));
  spin_buffer_kib = new QSpinBox;
  spin_buffer_kib->setRange(16, 4096);
  spin_buffer_kib->setSuffix(" " + tr("KiB"));
  // streamBufferSize() is in bytes; default 131072 = 128 KiB.
  int bytes = global_conf.streamBufferSize();
  if (bytes <= 0) bytes = 128 * BUFFER_BYTES_PER_KIB;
  spin_buffer_kib->setValue(bytes / BUFFER_BYTES_PER_KIB);
  buf_row->addWidget(spin_buffer_kib);
  buf_row->addStretch();
  vbox->addLayout(buf_row);

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

#ifdef ENABLE_GAPLESS
  // Gapless playback
  auto *gapless_row = new QHBoxLayout;
  check_gapless = new QCheckBox(tr("Enable gapless playback"));
  check_gapless->setChecked(!global_conf.disableGapless());
  gapless_row->addWidget(check_gapless);
  auto *gapless_hint = new QLabel(tr("(requires restart)"));
  gapless_hint->setStyleSheet("color: gray;");
  gapless_row->addWidget(gapless_hint);
  gapless_row->addStretch();
  vbox->addLayout(gapless_row);

  auto *gcache_row = new QHBoxLayout;
  gcache_row->addWidget(new QLabel(tr("Gapless memory buffer:")));
  spin_gapless_cache_mb = new QSpinBox;
  spin_gapless_cache_mb->setRange(1, 8192);
  spin_gapless_cache_mb->setSuffix(" " + tr("MB"));
  int gcache_mb = global_conf.gaplessCacheSizeMb();
  spin_gapless_cache_mb->setValue(gcache_mb > 0 ? gcache_mb : 100);
  spin_gapless_cache_mb->setEnabled(check_gapless->isChecked());
  gcache_row->addWidget(spin_gapless_cache_mb);
  auto *gcache_hint = new QLabel(tr("(requires restart)"));
  gcache_hint->setStyleSheet("color: gray;");
  gcache_row->addWidget(gcache_hint);
  gcache_row->addStretch();
  vbox->addLayout(gcache_row);

  auto *gcache_desc = new QLabel(tr(
    "Decoded audio kept in memory so track transitions are gapless and seeking "
    "within a track is instant. A larger buffer caches more (or longer) tracks; "
    "100 MB suits most libraries."));
  gcache_desc->setWordWrap(true);
  gcache_desc->setStyleSheet("color: gray;");
  vbox->addWidget(gcache_desc);

  connect(check_gapless, &QCheckBox::toggled, spin_gapless_cache_mb, &QWidget::setEnabled);
#endif

#ifdef MPRIS_ENABLE
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

#ifdef ENABLE_CRASH_HANDLER
  auto *crash_row = new QHBoxLayout;
  crash_row->addWidget(new QLabel(tr("Crash reports:")));
  combo_crash_reports = new QComboBox;
  combo_crash_reports->addItem(tr("Send automatically"), QStringLiteral("enabled"));
  combo_crash_reports->addItem(tr("Ask after next crash"), QString());
  combo_crash_reports->addItem(tr("Never send"), QStringLiteral("disabled"));
  const int crash_idx = combo_crash_reports->findData(local_conf.crashReportConsent());
  combo_crash_reports->setCurrentIndex(crash_idx >= 0 ? crash_idx : 1);
  crash_row->addWidget(combo_crash_reports);
  crash_row->addStretch();
  vbox->addLayout(crash_row);
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
  populateProviders(list_lyrics, Lyrics::ProviderChain::knownProviders(),
                    global_conf.lyricsProviders(), &Lyrics::ProviderChain::displayName);
}

void SettingsDialog::populateCovers() {
  populateProviders(list_covers, CoverArt::Online::ProviderChain::knownProviders(),
                    global_conf.coverProviders(), &CoverArt::Online::ProviderChain::displayName);
}

void SettingsDialog::populateProviders(QListWidget *list, const QStringList &known,
                                       const QStringList &configured,
                                       QString (*display_name)(const QString &)) {
  // Configured ones first (in order), then any known-but-disabled providers as
  // unchecked. Names the chain doesn't know are dropped, which is how legacy
  // built-in entries retire themselves on the next save.
  QStringList all;
  for (const auto &p : configured) {
    if (known.contains(p) && !all.contains(p)) all << p;
  }
  for (const auto &p : known) {
    if (!all.contains(p)) all << p;
  }
  for (const auto &p : all) {
    auto *item = new QListWidgetItem(display_name(p));
    item->setData(Qt::UserRole, p);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
                   Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    item->setCheckState(configured.contains(p) ? Qt::Checked : Qt::Unchecked);
    list->addItem(item);
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

QStringList SettingsDialog::collectProviders(const QListWidget *list) const {
  QStringList result;
  if (!list) return result;
  // Row order is the fallback order; unchecked means absent.
  for (int i = 0; i < list->count(); ++i) {
    auto *item = list->item(i);
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

  // Interface
  global_conf.saveTrayIconEnabled(check_tray_icon->isChecked());
  if (check_minimize_to_tray) {
    global_conf.saveMinimizeToTray(check_minimize_to_tray->isChecked());
  }
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
  global_conf.saveLyricsProviders(collectProviders(list_lyrics));
  global_conf.saveCoverProviders(collectProviders(list_covers));

  // Advanced
  global_conf.saveSingleInstance(check_single_instance->isChecked());
  global_conf.saveIpcPort(spin_ipc_port->value());
  global_conf.saveStreamBufferSize(spin_buffer_kib->value() * BUFFER_BYTES_PER_KIB);
  global_conf.savePlaybackLogSize(spin_playback_log_size->value());
#ifdef ENABLE_GAPLESS
  if (check_gapless) {
    global_conf.saveDisableGapless(!check_gapless->isChecked());
  }
  if (spin_gapless_cache_mb) {
    global_conf.saveGaplessCacheSizeMb(spin_gapless_cache_mb->value());
  }
#endif
#ifdef MPRIS_ENABLE
  if (list_mpris_blacklist) {
    global_conf.saveMprisBlacklist(collectMprisBlacklist());
  }
#endif
#ifdef ENABLE_MPD_SUPPORT
  if (check_mpd_stop_on_close) {
    global_conf.saveMpdStopPlayerOnClose(check_mpd_stop_on_close->isChecked());
  }
#endif
#ifdef ENABLE_CRASH_HANDLER
  if (combo_crash_reports) {
    local_conf.saveCrashReportConsent(combo_crash_reports->currentData().toString());
  }
#endif

  global_conf.sync();
  local_conf.sync();

  const bool tray_now = check_tray_icon->isChecked();
  if (tray_now != tray_was_enabled) {
    tray_was_enabled = tray_now;
    emit trayIconToggled();
  }
}

void SettingsDialog::onButtonBoxClicked(QAbstractButton *btn) {
  if (button_box->buttonRole(btn) == QDialogButtonBox::ApplyRole) {
    apply();
  } else if (button_box->buttonRole(btn) == QDialogButtonBox::AcceptRole) {
    apply();
  }
  // RejectRole: do nothing — accept()/reject() handle close.
}
