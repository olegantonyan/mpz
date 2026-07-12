#ifndef FEEDBACKFORM_H
#define FEEDBACKFORM_H

#include "loadingspinner.h"
#include "feedback_ui/feedbacksender.h"
#include "config/global.h"

#include <QDialog>

namespace Ui {
  class FeedbackForm;
}

class FeedbackForm : public QDialog {
  Q_OBJECT

public:
  explicit FeedbackForm(Config::Global &global, QWidget *parent = nullptr);
  ~FeedbackForm();

  void setCrashReport(const QString &crashText, const QString &crashId);

private slots:
  void on_pushButtonSend_clicked();

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  Ui::FeedbackForm *ui;
  LoadingSpinner *spinner;
  FeedbackSender sender;
  Config::Global &global_conf;
  bool done;
  bool crash_mode = false;
  QString crash_id;

  void send();
  void beginSend();
  void endSend(const QString &error);
  void finalize();
  void persistConsent();
};

#endif // FEEDBACKFORM_H
