#include "feedbackform.h"
#include "ui_feedbackform.h"
#include "sysinfo.h"

#include <QCloseEvent>

FeedbackForm::FeedbackForm(Config::Global &global, QWidget *parent) :
  QDialog(parent), ui(new Ui::FeedbackForm), global_conf(global), done(false) {
  ui->setupUi(this);

  ui->checkBoxSysinfo->setChecked(true);
  ui->lineEditSysinfo->setReadOnly(true);
  ui->lineEditSysinfo->setText(SysInfo::get().join(" | "));

  ui->checkBoxAutoCrashReport->setVisible(false);

  spinner = new LoadingSpinner(ui->pushButtonSend);
  spinner->hide();

  connect(&sender, &FeedbackSender::finished, this, [this](bool ok, const QString &error) {
    endSend(error);
    if (ok) {
      finalize();
    }
  });
}

FeedbackForm::~FeedbackForm() {
  delete ui;
}

void FeedbackForm::setCrashReport(const QString &crashText, const QString &crashId) {
  crash_mode = true;
  crash_id = crashId;
  setWindowTitle(tr("Crash report"));
  ui->label->setText(tr("mpz closed unexpectedly last time. Send this report to help fix it?"));
  ui->textEditText->setPlainText(crashText);
  ui->lineEditAuthor->setText(QStringLiteral("auto-crash-report"));
  ui->checkBoxAutoCrashReport->setChecked(global_conf.crashReportConsent() != QStringLiteral("disabled"));
  ui->checkBoxAutoCrashReport->setVisible(true);
}

void FeedbackForm::on_pushButtonSend_clicked() {
  if (done) {
    close();
    return;
  }
  send();
}

void FeedbackForm::send() {
  beginSend();
  const QString sysinfo = ui->checkBoxSysinfo->isChecked() ? ui->lineEditSysinfo->text() : QString();
  sender.submit(ui->textEditText->toPlainText(), ui->lineEditAuthor->text(), sysinfo);
}

void FeedbackForm::beginSend() {
  spinner->start();
  ui->pushButtonSend->setEnabled(false);
  ui->pushButtonSend->setText("");
}

void FeedbackForm::endSend(const QString &error) {
  ui->pushButtonSend->setEnabled(true);
  spinner->stop();
  if (!error.isEmpty()) {
    ui->pushButtonSend->setText(tr("Error occured, please try again") + "\n" + error);
  }
}

void FeedbackForm::finalize() {
  ui->pushButtonSend->setText(tr("Thanks for you feedback! (click again to close)"));
  ui->checkBoxSysinfo->setEnabled(false);
  ui->lineEditSysinfo->setEnabled(false);
  ui->textEditText->setEnabled(false);
  ui->lineEditAuthor->setEnabled(false);
  done = true;
}

void FeedbackForm::persistConsent() {
  if (!crash_mode) {
    return;
  }
  global_conf.saveCrashReportConsent(ui->checkBoxAutoCrashReport->isChecked()
    ? QStringLiteral("enabled") : QStringLiteral("disabled"));
  global_conf.saveLastReportedCrash(crash_id);
  global_conf.sync();
}

void FeedbackForm::closeEvent(QCloseEvent *event) {
  persistConsent();
  QDialog::closeEvent(event);
}
