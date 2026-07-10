#include "about_ui/aboutdialog.h"
#include "ui_aboutdialog.h"
#include "sysinfo.h"
#include "feedback_ui/feedbackform.h"

#include "about_ui/area51dialog.h"
#include <QEvent>
#include <QMouseEvent>

#if defined(ENABLE_UPDATE_CHECK)
  #include "update_check/updatechecker.h"
#endif

#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QDialog>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QDialogButtonBox>

AboutDialog::AboutDialog(Config::Global &global_c, QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog) {
  ui->setupUi(this);

  ui->versionLabel->setText(tr("Version %1").arg(qApp->applicationVersion()));
  ui->versionLabel->installEventFilter(this);

  ui->linksLabel->setTextFormat(Qt::RichText);
  ui->linksLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
  ui->linksLabel->setOpenExternalLinks(true);
  ui->linksLabel->setText(QString("<a href=\"https://mpz-player.org\">%1</a> &middot; "
                                  "<a href=\"https://github.com/olegantonyan/mpz\">%2</a>")
                          .arg(tr("Website"), tr("GitHub")));

#if defined(ENABLE_UPDATE_CHECK)
  if (!global_c.disableAutoUpdateCheck()) {
    ui->updateLabel->setTextFormat(Qt::RichText);
    ui->updateLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->updateLabel->setOpenExternalLinks(true);
    auto *checker = new UpdateChecker(this);
    connect(checker, &UpdateChecker::updateAvailable, this, [this](const QString &version, const QString &url) {
      ui->updateLabel->setText(tr("Update available:") + QString(" <a href=\"%1\">v%2</a>").arg(url, version));
      ui->updateLabel->show();
    });
    checker->check();
  }
#else
  Q_UNUSED(global_c)
#endif

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
  os << libraryInfo("yaml-cpp", "https://github.com/jbeder/yaml-cpp");
  os << libraryInfo("Bootstrap Icons", "https://icons.getbootstrap.com/");
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

bool AboutDialog::eventFilter(QObject *obj, QEvent *event) {
  if (obj == ui->versionLabel && event->type() == QEvent::MouseButtonPress &&
      (static_cast<QMouseEvent *>(event)->modifiers() & Qt::ShiftModifier)) {
    if (++version_shift_clicks_ >= 10) {
      version_shift_clicks_ = 0;
      Area51Dialog(this).exec();
    }
  }
  return QDialog::eventFilter(obj, event);
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
  browser->setMarkdown(markdown);
#else
  browser->setPlainText(markdown);
#endif

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
