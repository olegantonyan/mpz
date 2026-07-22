#include "equalizer_ui/equalizerdialog.h"

#include "eq/autoeqimport.h"
#include "eq/eqexport.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include <cmath>

namespace EqualizerUi {
  namespace {
    const double kGraphicFreqs[10] = {31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};

    int typeToIndex(Eq::Band::Type t) {
      switch (t) {
        case Eq::Band::Type::Peaking:   return 0;
        case Eq::Band::Type::LowShelf:  return 1;
        case Eq::Band::Type::HighShelf: return 2;
        case Eq::Band::Type::LowPass:   return 3;
        case Eq::Band::Type::HighPass:  return 4;
      }
      return 0;
    }

    Eq::Band::Type indexToType(int i) {
      switch (i) {
        case 1: return Eq::Band::Type::LowShelf;
        case 2: return Eq::Band::Type::HighShelf;
        case 3: return Eq::Band::Type::LowPass;
        case 4: return Eq::Band::Type::HighPass;
        default: return Eq::Band::Type::Peaking;
      }
    }
  }

  EqualizerDialog::EqualizerDialog(Playback::Controller *player, Config::Local &local_c,
                                   Config::Global &global_c, QWidget *parent) :
    QDialog(parent), player_(player), local_conf(local_c), global_conf(global_c) {
    setWindowTitle(tr("Equalizer"));

    profiles_ = local_conf.eqProfiles();
    if (profiles_.isEmpty()) {
      profiles_ << Eq::defaultGraphicProfile();
    }
    const QString active_name = local_conf.eqActiveProfile();
    for (int i = 0; i < profiles_.size(); ++i) {
      if (profiles_[i].name == active_name) {
        active_index_ = i;
        break;
      }
    }

    auto *root = new QVBoxLayout(this);

    enable_check_ = new QCheckBox(tr("Enable equalizer"));
    enable_check_->setChecked(local_conf.eqEnabled());
    connect(enable_check_, &QCheckBox::toggled, this, [this](bool) { applyCurrent(); });
    root->addWidget(enable_check_);

    auto *preset_box = new QGroupBox(tr("Preset"));
    auto *preset_layout = new QVBoxLayout(preset_box);

    preset_combo_ = new QComboBox;
    preset_combo_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(preset_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
      if (updating_ || idx < 0 || idx >= profiles_.size()) {
        return;
      }
      active_index_ = idx;
      loadActiveIntoWidgets();
      applyCurrent();
    });
    preset_layout->addWidget(preset_combo_);

    auto *actions = new QHBoxLayout;
    auto *save_btn = new QPushButton(tr("Save As…"));
    connect(save_btn, &QPushButton::clicked, this, &EqualizerDialog::onSaveAs);
    actions->addWidget(save_btn);
    auto *del_btn = new QPushButton(tr("Delete"));
    connect(del_btn, &QPushButton::clicked, this, &EqualizerDialog::onDeletePreset);
    actions->addWidget(del_btn);
    actions->addStretch();
    auto *import_btn = new QPushButton(tr("Import…"));
    import_btn->setToolTip(tr("Import an EqualizerAPO / AutoEQ ParametricEQ.txt file"));
    connect(import_btn, &QPushButton::clicked, this, &EqualizerDialog::onImport);
    actions->addWidget(import_btn);

    auto *export_btn = new QPushButton(tr("Export…"));
    auto *export_menu = new QMenu(export_btn);
    auto *export_param = export_menu->addAction(tr("Parametric EQ…"));
    connect(export_param, &QAction::triggered, this, [this]() { exportProfile(false); });
    auto *export_graphic = export_menu->addAction(tr("Graphic EQ (multiband)…"));
    connect(export_graphic, &QAction::triggered, this, [this]() { exportProfile(true); });
    export_btn->setMenu(export_menu);
    actions->addWidget(export_btn);
    preset_layout->addLayout(actions);

    assign_device_check_ = new QCheckBox(tr("Use automatically for the current output device"));
    const QString dev_key = QString::fromLatin1(local_conf.outputDeviceId().toHex());
    assign_device_check_->setEnabled(!dev_key.isEmpty());
    connect(assign_device_check_, &QCheckBox::toggled, this, [this](bool on) {
      if (updating_) {
        return;
      }
      const QString key = QString::fromLatin1(local_conf.outputDeviceId().toHex());
      if (key.isEmpty()) {
        return;
      }
      auto map = local_conf.eqDeviceProfiles();
      if (on) {
        map[key] = active().name;
      } else if (map.value(key) == active().name) {
        map.remove(key);
      }
      local_conf.saveEqDeviceProfiles(map);
    });
    preset_layout->addWidget(assign_device_check_);

    root->addWidget(preset_box);

    note_ = new QLabel(tr("The equalizer runs only on the gapless engine. Enable it in Settings."));
    note_->setWordWrap(true);
    note_->setStyleSheet("color: #d35400;");
    note_->setVisible(global_conf.disableGapless());
    root->addWidget(note_);

    auto *preamp_row = new QHBoxLayout;
    preamp_row->addWidget(new QLabel(tr("Preamp:")));
    preamp_spin_ = new QDoubleSpinBox;
    preamp_spin_->setRange(-30.0, 12.0);
    preamp_spin_->setSingleStep(0.1);
    preamp_spin_->setDecimals(1);
    preamp_spin_->setSuffix(" " + tr("dB"));
    connect(preamp_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
      if (updating_) {
        return;
      }
      active().preamp_db = v;
      applyCurrent();
    });
    preamp_row->addWidget(preamp_spin_);
    auto_preamp_check_ = new QCheckBox(tr("Auto (clip-safe)"));
    auto_preamp_check_->setToolTip(tr("Set the preamp so boosted bands never exceed 0 dBFS"));
    connect(auto_preamp_check_, &QCheckBox::toggled, this, [this](bool on) {
      if (updating_) {
        return;
      }
      active().auto_preamp = on;
      applyCurrent();
    });
    preamp_row->addWidget(auto_preamp_check_);
    preamp_row->addStretch();
    root->addLayout(preamp_row);

    curve_ = new CurveWidget;
    root->addWidget(curve_);

    tabs_ = new QTabWidget;
    tabs_->addTab(buildGraphicTab(), tr("Graphic"));
    tabs_->addTab(buildParametricTab(), tr("Parametric"));
    root->addWidget(tabs_, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    root->addWidget(buttons);

    rebuildPresetCombo();
    loadActiveIntoWidgets();
    applyCurrent();

    const int line = fontMetrics().height();
    resize(sizeHint().expandedTo(QSize(line * 46, line * 44)));
  }

  Eq::EqProfile &EqualizerDialog::active() {
    if (active_index_ < 0 || active_index_ >= profiles_.size()) {
      active_index_ = 0;
    }
    return profiles_[active_index_];
  }

  QWidget *EqualizerDialog::buildGraphicTab() {
    auto *page = new QWidget;
    auto *outer = new QVBoxLayout(page);
    auto *grid = new QGridLayout;

    for (int i = 0; i < 10; ++i) {
      auto *slider = new QSlider(Qt::Vertical);
      slider->setRange(-120, 120);
      slider->setTickPosition(QSlider::TicksBothSides);
      slider->setTickInterval(60);
      slider->setMinimumHeight(fontMetrics().height() * 9);
      connect(slider, &QSlider::valueChanged, this, [this, i](int v) {
        if (updating_ || !isGraphicForm(active())) {
          return;
        }
        active().bands[i].gain_db = v / 10.0;
        slider_labels_[i]->setText(QString::number(v / 10.0, 'f', 1));
        applyCurrent();
      });
      auto *value = new QLabel("0.0");
      value->setAlignment(Qt::AlignHCenter);
      QString hz = kGraphicFreqs[i] >= 1000 ? QString::number(kGraphicFreqs[i] / 1000.0, 'g', 3) + "k"
                                            : QString::number(kGraphicFreqs[i], 'g', 3);
      auto *freq = new QLabel(hz);
      freq->setAlignment(Qt::AlignHCenter);

      grid->addWidget(value, 0, i, Qt::AlignHCenter);
      grid->addWidget(slider, 1, i, Qt::AlignHCenter);
      grid->addWidget(freq, 2, i, Qt::AlignHCenter);

      sliders_ << slider;
      slider_labels_ << value;
    }
    outer->addLayout(grid);

    auto *bottom = new QHBoxLayout;
    graphic_hint_ = new QLabel(tr("This preset isn't a 10-band graphic EQ. Edit it in Parametric, or reset."));
    graphic_hint_->setWordWrap(true);
    bottom->addWidget(graphic_hint_, 1);
    auto *reset = new QPushButton(tr("Reset to 10-band"));
    connect(reset, &QPushButton::clicked, this, [this]() {
      const QString keep_name = active().name;
      Eq::EqProfile g = Eq::defaultGraphicProfile();
      g.name = keep_name;
      g.enabled = enable_check_->isChecked();
      g.auto_preamp = auto_preamp_check_->isChecked();
      active() = g;
      loadActiveIntoWidgets();
      applyCurrent();
    });
    bottom->addWidget(reset);
    outer->addLayout(bottom);

    return page;
  }

  QWidget *EqualizerDialog::buildParametricTab() {
    auto *page = new QWidget;
    auto *outer = new QVBoxLayout(page);

    table_ = new QTableWidget(0, 5);
    table_->setHorizontalHeaderLabels({tr("Type"), tr("Freq (Hz)"), tr("Gain (dB)"), "Q", tr("On")});
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    outer->addWidget(table_);

    auto *row = new QHBoxLayout;
    auto *add = new QPushButton(tr("Add band"));
    connect(add, &QPushButton::clicked, this, [this]() {
      Eq::Band b;
      b.freq_hz = 1000.0;
      b.gain_db = 0.0;
      b.q = 1.0;
      active().bands.push_back(b);
      syncTableFromProfile();
      syncGraphicFromProfile();
      applyCurrent();
    });
    row->addWidget(add);
    auto *remove = new QPushButton(tr("Remove selected"));
    connect(remove, &QPushButton::clicked, this, [this]() {
      const int r = table_->currentRow();
      if (r < 0 || r >= active().bands.size()) {
        return;
      }
      active().bands.remove(r);
      syncTableFromProfile();
      syncGraphicFromProfile();
      applyCurrent();
    });
    row->addWidget(remove);
    row->addStretch();
    outer->addLayout(row);

    return page;
  }

  void EqualizerDialog::addBandRow(const Eq::Band &band) {
    const int r = table_->rowCount();
    table_->insertRow(r);

    auto rebuild = [this]() {
      if (updating_) {
        return;
      }
      rebuildBandsFromTable();
      syncGraphicFromProfile();
      applyCurrent();
    };

    auto *type = new QComboBox;
    type->addItem(tr("Peak"));
    type->addItem(tr("Low shelf"));
    type->addItem(tr("High shelf"));
    type->addItem(tr("Low pass"));
    type->addItem(tr("High pass"));
    type->setCurrentIndex(typeToIndex(band.type));
    connect(type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [rebuild](int) { rebuild(); });
    table_->setCellWidget(r, 0, type);

    auto *freq = new QDoubleSpinBox;
    freq->setRange(10.0, 24000.0);
    freq->setDecimals(1);
    freq->setValue(band.freq_hz);
    connect(freq, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [rebuild](double) { rebuild(); });
    table_->setCellWidget(r, 1, freq);

    auto *gain = new QDoubleSpinBox;
    gain->setRange(-24.0, 24.0);
    gain->setSingleStep(0.1);
    gain->setDecimals(1);
    gain->setValue(band.gain_db);
    connect(gain, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [rebuild](double) { rebuild(); });
    table_->setCellWidget(r, 2, gain);

    auto *q = new QDoubleSpinBox;
    q->setRange(0.1, 10.0);
    q->setSingleStep(0.01);
    q->setDecimals(2);
    q->setValue(band.q);
    connect(q, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [rebuild](double) { rebuild(); });
    table_->setCellWidget(r, 3, q);

    auto *on = new QCheckBox;
    on->setChecked(band.enabled);
    connect(on, &QCheckBox::toggled, this, [rebuild](bool) { rebuild(); });
    table_->setCellWidget(r, 4, on);
  }

  void EqualizerDialog::rebuildBandsFromTable() {
    QVector<Eq::Band> bands;
    for (int r = 0; r < table_->rowCount(); ++r) {
      auto *type = qobject_cast<QComboBox *>(table_->cellWidget(r, 0));
      auto *freq = qobject_cast<QDoubleSpinBox *>(table_->cellWidget(r, 1));
      auto *gain = qobject_cast<QDoubleSpinBox *>(table_->cellWidget(r, 2));
      auto *q = qobject_cast<QDoubleSpinBox *>(table_->cellWidget(r, 3));
      auto *on = qobject_cast<QCheckBox *>(table_->cellWidget(r, 4));
      if (!type || !freq || !gain || !q || !on) {
        continue;
      }
      Eq::Band b;
      b.type = indexToType(type->currentIndex());
      b.freq_hz = freq->value();
      b.gain_db = gain->value();
      b.q = q->value();
      b.enabled = on->isChecked();
      bands << b;
    }
    active().bands = bands;
  }

  void EqualizerDialog::syncTableFromProfile() {
    updating_ = true;
    table_->setRowCount(0);
    for (const auto &b : active().bands) {
      addBandRow(b);
    }
    updating_ = false;
  }

  bool EqualizerDialog::isGraphicForm(const Eq::EqProfile &p) const {
    if (p.bands.size() != 10) {
      return false;
    }
    for (int i = 0; i < 10; ++i) {
      if (std::fabs(p.bands[i].freq_hz - kGraphicFreqs[i]) > kGraphicFreqs[i] * 0.02) {
        return false;
      }
    }
    return true;
  }

  void EqualizerDialog::syncGraphicFromProfile() {
    updating_ = true;
    const bool graphic = isGraphicForm(active());
    graphic_hint_->setVisible(!graphic);
    for (int i = 0; i < sliders_.size(); ++i) {
      sliders_[i]->setEnabled(graphic);
      if (graphic) {
        const int v = static_cast<int>(std::lround(active().bands[i].gain_db * 10.0));
        sliders_[i]->setValue(std::max(-120, std::min(120, v)));
        slider_labels_[i]->setText(QString::number(active().bands[i].gain_db, 'f', 1));
      } else {
        slider_labels_[i]->setText("--");
      }
    }
    updating_ = false;
  }

  void EqualizerDialog::rebuildPresetCombo() {
    updating_ = true;
    preset_combo_->clear();
    for (const auto &p : profiles_) {
      preset_combo_->addItem(p.name);
    }
    preset_combo_->setCurrentIndex(active_index_);
    updating_ = false;
  }

  void EqualizerDialog::loadActiveIntoWidgets() {
    updating_ = true;
    preset_combo_->setCurrentIndex(active_index_);
    auto_preamp_check_->setChecked(active().auto_preamp);
    preamp_spin_->setValue(active().preamp_db);
    const QString key = QString::fromLatin1(local_conf.outputDeviceId().toHex());
    assign_device_check_->setChecked(!key.isEmpty() &&
                                     local_conf.eqDeviceProfiles().value(key) == active().name);
    updating_ = false;

    syncGraphicFromProfile();
    syncTableFromProfile();
    refreshPreampDisplay();
  }

  void EqualizerDialog::refreshPreampDisplay() {
    updating_ = true;
    if (auto_preamp_check_->isChecked()) {
      preamp_spin_->setEnabled(false);
      preamp_spin_->setValue(curve_->autoPreampDb());
    } else {
      preamp_spin_->setEnabled(true);
    }
    updating_ = false;
  }

  void EqualizerDialog::applyCurrent() {
    if (updating_) {
      return;
    }
    Eq::EqProfile p = active();
    p.enabled = enable_check_->isChecked();
    player_->setEqualizer(p);
    curve_->setProfile(p, display_fs_);
    refreshPreampDisplay();

    local_conf.saveEqProfiles(profiles_);
    local_conf.saveEqEnabled(enable_check_->isChecked());
    local_conf.saveEqActiveProfile(active().name);
  }

  void EqualizerDialog::onSaveAs() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Save preset"), tr("Preset name:"),
                                               QLineEdit::Normal, active().name, &ok).trimmed();
    if (!ok || name.isEmpty()) {
      return;
    }
    Eq::EqProfile p = active();
    p.name = name;
    int existing = -1;
    for (int i = 0; i < profiles_.size(); ++i) {
      if (profiles_[i].name == name) {
        existing = i;
        break;
      }
    }
    if (existing >= 0) {
      profiles_[existing] = p;
      active_index_ = existing;
    } else {
      profiles_ << p;
      active_index_ = profiles_.size() - 1;
    }
    rebuildPresetCombo();
    loadActiveIntoWidgets();
    applyCurrent();
  }

  void EqualizerDialog::onDeletePreset() {
    profiles_.removeAt(active_index_);
    if (profiles_.isEmpty()) {
      profiles_ << Eq::defaultGraphicProfile();
    }
    active_index_ = std::max(0, std::min(active_index_, static_cast<int>(profiles_.size()) - 1));
    rebuildPresetCombo();
    loadActiveIntoWidgets();
    applyCurrent();
  }

  void EqualizerDialog::onImport() {
    QDialog help(this);
    help.setWindowTitle(tr("Import equalizer preset"));
    auto *v = new QVBoxLayout(&help);

    auto *label = new QLabel(&help);
    label->setTextFormat(Qt::RichText);
    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setWordWrap(true);
    label->setText(QStringLiteral(
      "<p>mpz reads the <b>EqualizerAPO / AutoEQ</b> text format: a <tt>Preamp:</tt> line "
      "plus <tt>Filter N: ON PK Fc … Gain … Q …</tt> lines. Download a headphone "
      "correction preset, then choose the file below.</p>"
      "<p><b>Where to download</b> — pick the <b>Parametric EQ</b> file "
      "(not GraphicEQ or convolution):</p>"
      "<ul>"
      "<li><a href=\"https://autoeq.app\">autoeq.app</a> — choose your headphone and target, then download</li>"
      "<li><a href=\"https://github.com/jaakkopasanen/AutoEq\">AutoEq database</a> — "
      "thousands of results (<tt>… ParametricEQ.txt</tt>)</li>"
      "<li><a href=\"https://www.reddit.com/r/oratory1990\">r/oratory1990</a> — "
      "hand-tuned EqualizerAPO configs</li>"
      "<li><a href=\"https://squig.link\">squig.link</a> / "
      "<a href=\"https://crinacle.com\">crinacle.com</a> — export &quot;Parametric EQ&quot;</li>"
      "</ul>"
      "<p>The preset is added as a new entry named after the file. Its preamp is used if "
      "present; otherwise <b>Auto (clip-safe)</b> stays on.</p>"));
    label->setMinimumWidth(fontMetrics().averageCharWidth() * 64);
    v->addWidget(label);

    auto *box = new QDialogButtonBox(&help);
    box->addButton(tr("Choose file…"), QDialogButtonBox::AcceptRole);
    box->addButton(QDialogButtonBox::Cancel);
    connect(box, &QDialogButtonBox::accepted, &help, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &help, &QDialog::reject);
    v->addWidget(box);

    if (help.exec() != QDialog::Accepted) {
      return;
    }
    importFromFile();
  }

  void EqualizerDialog::importFromFile() {
    const QString path = QFileDialog::getOpenFileName(
      this, tr("Import EqualizerAPO / AutoEQ preset"), QString(),
      tr("Parametric EQ (*.txt);;All files (*)"));
    if (path.isEmpty()) {
      return;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QMessageBox::warning(this, tr("Import failed"), tr("Could not open the file."));
      return;
    }
    const Eq::ParsedEq parsed = Eq::parseParametricEq(QString::fromUtf8(f.readAll()));
    if (parsed.bands.empty()) {
      QMessageBox::warning(this, tr("Import failed"), tr("No filters found in the file."));
      return;
    }

    Eq::EqProfile p;
    p.name = QFileInfo(path).completeBaseName();
    p.enabled = true;
    p.auto_preamp = !parsed.preamp_present;
    p.preamp_db = parsed.preamp_present ? parsed.preamp_db : 0.0;
    for (const auto &b : parsed.bands) {
      p.bands.push_back(b);
    }

    int existing = -1;
    for (int i = 0; i < profiles_.size(); ++i) {
      if (profiles_[i].name == p.name) {
        existing = i;
        break;
      }
    }
    if (existing >= 0) {
      profiles_[existing] = p;
      active_index_ = existing;
    } else {
      profiles_ << p;
      active_index_ = profiles_.size() - 1;
    }
    enable_check_->setChecked(true);
    rebuildPresetCombo();
    loadActiveIntoWidgets();
    applyCurrent();
  }

  void EqualizerDialog::exportProfile(bool graphic) {
    const QString suggested =
      active().name + (graphic ? " GraphicEQ" : " ParametricEQ") + ".txt";
    const QString path = QFileDialog::getSaveFileName(
      this, tr("Export preset"), suggested, tr("Text files (*.txt);;All files (*)"));
    if (path.isEmpty()) {
      return;
    }
    const QString text = graphic ? Eq::exportGraphicEq(active(), display_fs_)
                                 : Eq::exportParametricEq(active());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QMessageBox::warning(this, tr("Export failed"), tr("Could not write the file."));
      return;
    }
    f.write(text.toUtf8());
  }

  void EqualizerDialog::closeEvent(QCloseEvent *event) {
    local_conf.sync();
    QDialog::closeEvent(event);
  }
}
