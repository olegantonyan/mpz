#ifndef ADDMPZDDIALOG_H
#define ADDMPZDDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QWidget>

namespace DirectoryUi {
  class AddMpzdDialog : public QDialog {
    Q_OBJECT
  public:
    explicit AddMpzdDialog(QWidget *parent = nullptr);

    QString url() const;
    void setUrl(const QString &arg);

  private:
    QLineEdit *host_lineedit;
    QLabel *test_label;

  private slots:
    void onTestConnection();
  };
}

#endif // ADDMPZDDIALOG_H
