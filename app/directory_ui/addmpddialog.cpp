#include "addmpddialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>

namespace DirectoryUi {
  AddMpdDialog::AddMpdDialog(MpdClient::Client &cl, QWidget *parent) : QDialog{parent}, client(cl) {
    setWindowTitle(tr("Add mpd connection"));
    resize(500, 250);

    auto *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("host:port"));
    host_lineedit = new QLineEdit("localhost:6600");
    layout->addWidget(host_lineedit);

    layout->addWidget(new QLabel(tr("password (optional)")));
    password_lineedit = new QLineEdit;
    // password_lineedit->setEchoMode(QLineEdit::Password);
    layout->addWidget(password_lineedit);

    auto *test_row = new QHBoxLayout;

    test_button = new QPushButton(tr("Test connection"));
    test_row->addWidget(test_button);

    test_label = new QLabel;
    test_row->addWidget(test_label);

    test_row->addStretch();
    layout->addLayout(test_row);

    connect(test_button, &QPushButton::clicked, this, &AddMpdDialog::onTestConnection);

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(bb);

    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
  }

  QString AddMpdDialog::url() const {
    auto text = host_lineedit->text();
    if (!text.isEmpty()) {
      if (!text.startsWith("mpd://")) {
        text.prepend("mpd://");
      }
    }
    QUrl url(text);
    if (!password().isEmpty()) {
      url.setUserName("");
      url.setPassword(password());
    }
    return url.toString();
  }

  QString AddMpdDialog::password() const {
    return password_lineedit->text();
  }

  void AddMpdDialog::onTestConnection() {
    auto result = client.probe(QUrl(url()));
    if (result.first) {
      test_label->setText(QString("%1: %2").arg(tr("Success")).arg(result.second));
    } else {
      test_label->setText(QString("%1: %2").arg(tr("Failure")).arg(result.second));
    }
  }
}
