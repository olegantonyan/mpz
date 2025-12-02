#ifndef ADDMPDDIALOG_H
#define ADDMPDDIALOG_H

#include "mpd_client/client.h"

#include <QDialog>
#include <QObject>
#include <QWidget>
#include <QLineEdit>
#include <QLabel>

namespace DirectoryUi {
  class AddMpdDialog : public QDialog {
    Q_OBJECT
  public:
    explicit AddMpdDialog(MpdClient::Client &cl, QWidget *parent = nullptr);

    QString url() const;
    QString password() const;

  private:
    MpdClient::Client &client;
    QLineEdit *host_lineedit;
    QLineEdit *password_lineedit;
    QPushButton *test_button;
    QLabel *test_label;

  private slots:
    void onTestConnection();
  };
}

#endif // ADDMPDDIALOG_H
