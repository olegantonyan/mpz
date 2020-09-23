#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

namespace Ui {
  class AboutDialog;
}

class AboutDialog : public QDialog {
  Q_OBJECT

public:
  explicit AboutDialog(QWidget *parent = nullptr);
  ~AboutDialog();

private slots:
  void on_buttonAboutQt_clicked();

private:
  Ui::AboutDialog *ui;

  QString libraryInfo(const QString &name, const QString &url) const;
};

#endif // ABOUTDIALOG_H
