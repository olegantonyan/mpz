#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "config/global.h"

#include <QDialog>

namespace Ui {
  class AboutDialog;
}

class AboutDialog : public QDialog {
  Q_OBJECT
public:
  explicit AboutDialog(Config::Global &global_c, QWidget *parent = nullptr);
  ~AboutDialog();

  static void show_changelog();

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
  void on_buttonAboutQt_clicked() const;

  void on_buttonContact_clicked() const;

  void on_buttonChangelog_clicked() const;

private:
  Ui::AboutDialog *ui;

  int version_shift_clicks_ = 0;

  QString libraryInfo(const QString &name, const QString &url) const;
};

#endif // ABOUTDIALOG_H
