#include "feedbackform.h"
#include "ui_feedbackform.h"
#include "sysinfo.h"

#include <QDebug>
#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonObject>

FeedbackForm::FeedbackForm(QWidget *parent) : QDialog(parent), ui(new Ui::FeedbackForm), done(false) {
  ui->setupUi(this);

  ui->checkBoxSysinfo->setChecked(true);
  ui->lineEditSysinfo->setReadOnly(true);
  ui->lineEditSysinfo->setText(SysInfo::get().join(" | "));

  spinner = new WaitingSpinnerWidget(ui->pushButtonSend);
  spinner->setRoundness(70.0);
  spinner->setMinimumTrailOpacity(15.0);
  spinner->setTrailFadePercentage(70.0);
  spinner->setNumberOfLines(7);
  spinner->setLineLength(6);
  spinner->setLineWidth(3);
  spinner->setInnerRadius(3);
  spinner->setRevolutionsPerSecond(1);
  spinner->setColor(QColor(120, 120, 120));
  spinner->hide();
}

FeedbackForm::~FeedbackForm() {
  delete ui;
}

void FeedbackForm::finalize() {
  ui->pushButtonSend->setText(tr("Thanks for you feedback! (click again to close)"));
  ui->checkBoxSysinfo->setEnabled(false);
  ui->lineEditSysinfo->setEnabled(false);
  ui->textEditText->setEnabled(false);
  ui->lineEditAuthor->setEnabled(false);
  done = true;
}

void FeedbackForm::on_pushButtonSend_clicked() {
  if (done) {
    close();
    return;
  }

  if (!send()) {
    return;
  }

  finalize();
}

void FeedbackForm::beginSend() {
  spinner->show();
  spinner->start();
  ui->pushButtonSend->setEnabled(false);
  ui->pushButtonSend->setText("");
}

void FeedbackForm::endSend(const QString &error) {
  ui->pushButtonSend->setEnabled(true);
  spinner->stop();
  spinner->hide();
  if (!error.isEmpty()) {
    ui->pushButtonSend->setText(tr("Error occured, please try again") + "\n" + error);
  }
}

QByteArray FeedbackForm::buildJson() {
  QJsonObject json;
  json["text"] = ui->textEditText->toPlainText();
  json["author"] = ui->lineEditAuthor->text();
  if (ui->checkBoxSysinfo->isChecked()) {
    json["sysinfo"] = ui->lineEditSysinfo->text();
  }
  return QJsonDocument(json).toJson();
}

bool FeedbackForm::send() {
  beginSend();

  QUrl url("https://us-central1-random-360814.cloudfunctions.net/mpz-feedback");
#ifdef DISABLE_HTTPS
  url.setScheme("http"); // TODO: https make openssl static build for windows
#endif
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QNetworkAccessManager nam;
  QNetworkReply *reply = nam.post(request, buildJson());
  while (!reply->isFinished()) {
    qApp->processEvents();
  }

  bool ok = reply->error() == QNetworkReply::NoError;
  endSend(ok ? "" : reply->errorString());

  reply->deleteLater();

  return ok;
}
