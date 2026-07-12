#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "config/global.h"
#include "config/local.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QListWidget;
class QTableWidget;
class QPushButton;
class QDialogButtonBox;
class QAbstractButton;

class SettingsDialog : public QDialog {
  Q_OBJECT
public:
  explicit SettingsDialog(Config::Global &global_c, Config::Local &local_c, QWidget *parent = nullptr);

signals:
  void trayIconToggled();

private:
  Config::Global &global_conf;
  Config::Local &local_conf;

  QCheckBox *check_stop_when_track_removed = nullptr;
  QCheckBox *check_inhibit_sleep = nullptr;
  QSpinBox *spin_buffer_kib = nullptr;

  QCheckBox *check_tray_icon = nullptr;
  QCheckBox *check_minimize_to_tray = nullptr;
  QCheckBox *check_auto_update = nullptr;
  QCheckBox *check_row_height = nullptr;
  QSpinBox *spin_row_height = nullptr;
  QComboBox *combo_language = nullptr;

  QComboBox *combo_library_filter_scope = nullptr;

  QTableWidget *table_columns = nullptr;
  QListWidget *list_lyrics = nullptr;

  QCheckBox *check_single_instance = nullptr;
  QSpinBox *spin_ipc_port = nullptr;
  QSpinBox *spin_playback_log_size = nullptr;

  QListWidget *list_mpris_blacklist = nullptr;
  QCheckBox *check_mpd_stop_on_close = nullptr;
  QComboBox *combo_crash_reports = nullptr;

  QDialogButtonBox *button_box = nullptr;

  bool tray_was_enabled = false;

  QWidget *buildGeneralTab();
  QWidget *buildLyricsTab();
  QWidget *buildAdvancedTab();

  void populateLanguages();
  void populateColumns();
  void populateLyrics();
  void populateMprisBlacklist();

  void fitColumnsTableHeight();

  PlaylistUi::ColumnsConfig collectColumns() const;
  QStringList collectLyrics() const;
  QStringList collectMprisBlacklist() const;

  void apply();
  void onButtonBoxClicked(QAbstractButton *btn);
};

#endif // SETTINGSDIALOG_H
