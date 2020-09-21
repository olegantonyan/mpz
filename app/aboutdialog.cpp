#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog) {
  ui->setupUi(this);
  setWindowTitle("About mpz");

  auto url = "https://github.com/olegantonyan/mpz";
  auto version = qApp->applicationVersion();

  ui->infoLabel->setTextFormat(Qt::RichText);
  ui->infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  ui->infoLabel->setOpenExternalLinks(true);
  ui->infoLabel->setText(QString("version %1 (%2) <a href=\"%3\">%4</a>").arg(version).arg(GIT_CURRENT_SHA1).arg(url).arg(url));

  ui->opensourceLabel->setTextFormat(Qt::RichText);
  ui->opensourceLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  ui->opensourceLabel->setOpenExternalLinks(true);

  QStringList os;
  os << libraryInfo("Qt", "https://www.qt.io/");
  os << libraryInfo("TagLib", "https://taglib.org/");
  os << libraryInfo("QHotKey", "https://github.com/Skycoder42/QHotkey");
  os << libraryInfo("QtWaitingSpinner", "https://github.com/snowwlex/QtWaitingSpinner");
  os << libraryInfo("yaml-cpp", "https://github.com/jbeder/yaml-cpp");
  ui->opensourceLabel->setText(QString("Using opensource libraries:<br /> %1").arg(os.join("<br />")));
}

AboutDialog::~AboutDialog() {
  delete ui;
}

QString AboutDialog::libraryInfo(const QString &name, const QString &url) const {
  return QString("%1 <a href=\"%2\">%3</a>").arg(name).arg(url).arg(url);
}
