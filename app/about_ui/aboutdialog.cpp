#include "about_ui/aboutdialog.h"
#include "ui_aboutdialog.h"
#include "sysinfo.h"
#include "feedback_ui/feedbackform.h"
#include "config/storage.h"

#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QTextStream>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog) {
  ui->setupUi(this);

  auto url = "https://github.com/olegantonyan/mpz";
  auto version = qApp->applicationVersion();

  ui->infoLabel->setTextFormat(Qt::RichText);
  ui->infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  ui->infoLabel->setOpenExternalLinks(true);
  ui->infoLabel->setText(QString("<a href=\"%1\">%2</a>").arg(url).arg(url));

  ui->opensourceLabel->setTextFormat(Qt::RichText);
  ui->opensourceLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  ui->opensourceLabel->setOpenExternalLinks(true);

  QStringList os;
  os << libraryInfo("Qt", "https://www.qt.io/");
  os << libraryInfo("TagLib", "https://taglib.org/");
  os << libraryInfo("QHotKey", "https://github.com/Skycoder42/QHotkey");
  os << libraryInfo("QtWaitingSpinner", "https://github.com/snowwlex/QtWaitingSpinner");
  os << libraryInfo("yaml-cpp", "https://github.com/jbeder/yaml-cpp");
  ui->opensourceLabel->setText(tr("Using opensource libraries") + QString(":<br /> %1").arg(os.join("<br />")));

  ui->sysinfo->setText(SysInfo::get().join("<br />"));

  ui->configfileLabel->setText(tr("Config files path: ") + Config::Storage::configPath());
}

AboutDialog::~AboutDialog() {
  delete ui;
}

void AboutDialog::show_changelog() {
  QFile file(":/CHANGELOG.md");
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "error opening changelog resource file";
    return;
  }
  QStringList lines;
  int sections = 0;
  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine();
    if (line.startsWith("## ")) {
      sections++;
    }
    if (sections > 3) {
      break;
    }
    lines << line;
  }
  lines << "<a href='https://github.com/olegantonyan/mpz/blob/master/CHANGELOG.md'>Full changelog</a>";

  QMessageBox msg;
  msg.setTextInteractionFlags(Qt::TextBrowserInteraction);
  msg.setTextFormat(Qt::RichText);
  msg.setWindowTitle(tr("Changelog"));
  msg.setText(lines.join("<br />"));
  msg.exec();
}

QString AboutDialog::libraryInfo(const QString &name, const QString &url) const {
  return QString("%1 <a href=\"%2\">%3</a>").arg(name).arg(url).arg(url);
}

void AboutDialog::on_buttonAboutQt_clicked() const {
  qApp->aboutQt();
}

void AboutDialog::on_buttonContact_clicked() const {
  FeedbackForm().exec();
}

void AboutDialog::on_buttonChangelog_clicked() const {
  AboutDialog::show_changelog();
}
