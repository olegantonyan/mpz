#ifndef FEEDBACKFORM_H
#define FEEDBACKFORM_H

#include "waitingspinnerwidget.h"

#include <QDialog>

namespace Ui {
  class FeedbackForm;
}

class FeedbackForm : public QDialog {
  Q_OBJECT

public:
  explicit FeedbackForm(QWidget *parent = nullptr);
  ~FeedbackForm();

private slots:
  void on_pushButtonSend_clicked();

private:
  Ui::FeedbackForm *ui;
  WaitingSpinnerWidget *spinner;
  bool done;

  bool send();
  void beginSend();
  void endSend(const QString &error);
  void finalize();
  QByteArray buildJson();
};

#endif // FEEDBACKFORM_H
