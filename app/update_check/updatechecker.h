#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

class QNetworkReply;

class UpdateChecker : public QObject {
  Q_OBJECT
public:
  explicit UpdateChecker(QObject *parent = nullptr);

  void check();

  static bool isNewer(const QString &remoteTag, const QString &localVersion);

signals:
  void updateAvailable(const QString &version, const QString &url);

private:
  void fetch();
  void handleReply(QNetworkReply *reply);
  void emitIfNewer(const QString &version, const QString &url);

  QString cachePath() const;
  bool readCache(qint64 &last_check, QString &version, QString &url) const;
  void writeCache(const QString &version, const QString &url);

  QNetworkAccessManager nam;
};

#endif // UPDATECHECKER_H
