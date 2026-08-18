#ifndef FEEDBACKSENDER_H
#define FEEDBACKSENDER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>

class FeedbackSender : public QObject {
  Q_OBJECT
public:
  explicit FeedbackSender(QObject *parent = nullptr);

  void submit(const QString &text, const QString &author, const QString &sysinfo);

signals:
  void finished(bool ok, const QString &error);

private:
  QNetworkAccessManager nam;
};

#endif // FEEDBACKSENDER_H
