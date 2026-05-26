#include "about_ui/aboutdialog.h"
#include "ui_aboutdialog.h"
#include "sysinfo.h"
#include "feedback_ui/feedbackform.h"

#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QDialog>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QDialogButtonBox>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog) {
  ui->setupUi(this);

  ui->versionLabel->setText(tr("Version %1").arg(qApp->applicationVersion()));

  ui->linksLabel->setTextFormat(Qt::RichText);
  ui->linksLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  ui->linksLabel->setOpenExternalLinks(true);
  ui->linksLabel->setText(QString("<a href=\"https://mpz-player.org\">%1</a> &middot; "
                                  "<a href=\"https://github.com/olegantonyan/mpz\">%2</a>")
                          .arg(tr("Website"), tr("GitHub")));

  ui->copyrightLabel->setTextFormat(Qt::RichText);
  ui->copyrightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  ui->copyrightLabel->setOpenExternalLinks(true);
  ui->copyrightLabel->setText(QString("&copy; "
                                      "<a href=\"https://github.com/olegantonyan/mpz/blob/master/license.txt\">GPLv3</a>"));

  ui->opensourceLabel->setTextFormat(Qt::RichText);
  ui->opensourceLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  ui->opensourceLabel->setOpenExternalLinks(true);

  QStringList os;
  os << libraryInfo("Qt", "https://www.qt.io/");
  os << libraryInfo("TagLib", "https://taglib.org/");
  os << libraryInfo("QHotKey", "https://github.com/Skycoder42/QHotkey");
  os << libraryInfo("QtWaitingSpinner", "https://github.com/snowwlex/QtWaitingSpinner");
  os << libraryInfo("yaml-cpp", "https://github.com/jbeder/yaml-cpp");
#ifdef ENABLE_MPD_SUPPORT
  os << libraryInfo("libmpdclient", "https://github.com/MusicPlayerDaemon/libmpdclient");
#endif
#ifdef ENABLE_CRASH_HANDLER
  os << libraryInfo("cpptrace", "https://github.com/jeremy-rifkin/cpptrace");
#endif
  ui->opensourceLabel->setText(os.join(" &middot; "));

  ui->sysinfo->setText(SysInfo::get().join("<br />"));
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
  QTextStream in(&file);
  QString markdown = in.readAll();
  markdown += "\n\n[Full changelog](https://github.com/olegantonyan/mpz/blob/master/CHANGELOG.md)\n";

  QDialog dialog;
  dialog.setWindowTitle(tr("Changelog"));
  dialog.resize(500, 600);

  QTextBrowser *browser = new QTextBrowser(&dialog);
  browser->setOpenExternalLinks(true);
  browser->setMarkdown(markdown);

  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  layout->addWidget(browser);
  layout->addWidget(buttons);

  dialog.exec();
}

QString AboutDialog::libraryInfo(const QString &name, const QString &url) const {
  return QString("<a href=\"%1\">%2</a>").arg(url, name);
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
