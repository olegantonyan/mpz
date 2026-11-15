#include "addmpzddialog.h"
#include "mpzd_client/client.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace DirectoryUi {
  AddMpzdDialog::AddMpzdDialog(QWidget *parent) : QDialog{parent} {
    setWindowTitle(tr("Add mpzd connection"));
    resize(500, 160);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("host:port"));
    host_lineedit = new QLineEdit(QString("localhost:%1").arg(MpzdClient::DEFAULT_PORT));
    layout->addWidget(host_lineedit);

    auto *test_row = new QHBoxLayout;
    auto *test_button = new QPushButton(tr("Test connection"));
    test_row->addWidget(test_button);
    test_label = new QLabel;
    test_row->addWidget(test_label);
    test_row->addStretch();
    layout->addLayout(test_row);
    connect(test_button, &QPushButton::clicked, this, &AddMpzdDialog::onTestConnection);

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
  }

  QString AddMpzdDialog::url() const {
    auto text = host_lineedit->text().trimmed();
    if (text.isEmpty()) {
      return QString();
    }
    if (!text.startsWith("mpzd://")) {
      text.prepend("mpzd://");
    }
    return QUrl(text).toString();
  }

  void AddMpzdDialog::setUrl(const QString &arg) {
    QUrl url(arg);
    QString hostport = url.host();
    if (url.port() != -1) {
      hostport += ":" + QString::number(url.port());
    }
    host_lineedit->setText(hostport);
  }

  void AddMpzdDialog::onTestConnection() {
    test_label->setText(tr("Connecting..."));
    test_label->repaint();
    const auto result = MpzdClient::Client::probe(QUrl(url()));
    test_label->setText(QString("%1: %2").arg(result.first ? tr("Success") : tr("Failure"), result.second));
  }
}
