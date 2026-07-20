#include "radiostationdialog.h"
#include "radio/resolver.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QUuid>

namespace DirectoryUi {
  RadioStationDialog::RadioStationDialog(const Radio::Station &station, QWidget *parent) :
    QDialog(parent), _station(station) {
    setWindowTitle(_station.id.isEmpty() ? tr("Add radio station") : tr("Edit radio station"));

    name_edit = new QLineEdit(_station.name);
    url_edit = new QLineEdit(_station.url);
    url_edit->setPlaceholderText(tr("stream url, or a .pls / .m3u link"));
    group_edit = new QLineEdit(_station.group);
    description_edit = new QLineEdit(_station.description);
    homepage_edit = new QLineEdit(_station.homepage);
    logo_edit = new QLineEdit(_station.logo_url);
    codec_edit = new QLineEdit(_station.codec);
    codec_edit->setPlaceholderText(tr("mp3, aac, ..."));
    bitrate_spin = new QSpinBox;
    bitrate_spin->setRange(0, 2048);
    bitrate_spin->setSuffix(" " + tr("kbps"));
    bitrate_spin->setSpecialValueText(tr("unspecified"));
    bitrate_spin->setValue(_station.bitrate);

    auto *browse = new QPushButton(tr("From file..."));
    connect(browse, &QPushButton::clicked, this, &RadioStationDialog::browseForPlaylist);
    auto *url_row = new QHBoxLayout;
    url_row->addWidget(url_edit);
    url_row->addWidget(browse);

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->addRow(tr("Name"), name_edit);
    form->addRow(tr("Stream"), url_row);
    form->addRow(tr("Group"), group_edit);
    form->addRow(tr("Description"), description_edit);
    form->addRow(tr("Homepage"), homepage_edit);
    form->addRow(tr("Logo URL"), logo_edit);
    form->addRow(tr("Codec"), codec_edit);
    form->addRow(tr("Bitrate"), bitrate_spin);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &RadioStationDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &RadioStationDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
    resize(480, sizeHint().height());
  }

  void RadioStationDialog::browseForPlaylist() {
    const auto file = QFileDialog::getOpenFileName(
      this, tr("Open playlist"), QString(), tr("Playlists (*.pls *.m3u *.m3u8);;All files (*)"));
    if (!file.isEmpty()) {
      url_edit->setText(file);
    }
  }

  void RadioStationDialog::accept() {
    if (name_edit->text().trimmed().isEmpty()) {
      QMessageBox::warning(this, windowTitle(), tr("A name is required."));
      return;
    }

    QString error;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const auto url = Radio::resolveStreamUrl(url_edit->text(), &error);
    QApplication::restoreOverrideCursor();
    if (url.isEmpty()) {
      QMessageBox::warning(this, windowTitle(), tr("Could not use this stream: %1").arg(error));
      return;
    }

    _station.name = name_edit->text().trimmed();
    _station.url = url;
    _station.group = group_edit->text().trimmed();
    _station.description = description_edit->text().trimmed();
    _station.homepage = homepage_edit->text().trimmed();
    _station.logo_url = logo_edit->text().trimmed();
    _station.codec = codec_edit->text().trimmed().toLower();
    _station.bitrate = static_cast<quint16>(bitrate_spin->value());
    if (_station.id.isEmpty()) {
      _station.id = QStringLiteral("user-") + QUuid::createUuid().toString(QUuid::Id128);
    }

    QDialog::accept();
  }

  Radio::Station RadioStationDialog::station() const {
    return _station;
  }
}
