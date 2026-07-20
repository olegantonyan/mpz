#include "radio/catalog.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QUrl>

namespace Radio {
  namespace {
    const char *kBuiltinResource = ":/app/resources/radio/radio.json";

    QString &userPathRef() {
      static QString path;
      return path;
    }

    QString &lastErrorRef() {
      static QString error;
      return error;
    }

    Catalog *&cacheRef() {
      static Catalog *cache = nullptr;
      return cache;
    }

    Station parseStation(const QJsonObject &o) {
      Station st;
      st.id = o.value("id").toString().trimmed();
      st.name = o.value("name").toString().trimmed();
      st.group = o.value("group").toString().trimmed();
      st.description = o.value("description").toString().trimmed();
      st.url = o.value("url").toString().trimmed();
      st.codec = o.value("codec").toString().trimmed().toLower();
      st.bitrate = static_cast<quint16>(o.value("bitrate").toInt());
      st.homepage = o.value("homepage").toString().trimmed();
      st.logo_url = o.value("logo_url").toString().trimmed();
      return st;
    }
  }

  const QVector<Station> &Catalog::stations() const {
    return _stations;
  }

  // Ungrouped stations are deliberately not represented here -- they render at
  // the top level rather than under a folder.
  QStringList Catalog::groups() const {
    QStringList result;
    for (const auto &st : _stations) {
      if (!st.group.isEmpty() && !result.contains(st.group)) {
        result << st.group;
      }
    }
    return result;
  }

  const Station *Catalog::byId(const QString &id) const {
    for (const auto &st : _stations) {
      if (st.id == id) {
        return &st;
      }
    }
    return nullptr;
  }

  bool Catalog::extendsBuiltin() const {
    return _extends_builtin;
  }

  Catalog Catalog::fromJson(const QByteArray &json, QString *error) {
    const auto fail = [error](const QString &message) {
      if (error) {
        *error = message;
      }
      return Catalog();
    };

    QJsonParseError parse_error{};
    const auto doc = QJsonDocument::fromJson(json, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
      return fail(QObject::tr("%1 (at offset %2)")
                    .arg(parse_error.errorString())
                    .arg(parse_error.offset));
    }
    if (!doc.isObject()) {
      return fail(QObject::tr("expected a JSON object at the top level"));
    }

    const auto root = doc.object();
    if (!root.value("stations").isArray()) {
      return fail(QObject::tr("missing a \"stations\" array"));
    }

    Catalog result;
    result._extends_builtin = root.value("extends_builtin").toBool(false);

    QSet<QString> seen_ids;
    const auto stations = root.value("stations").toArray();
    for (int i = 0; i < stations.size(); i++) {
      if (!stations.at(i).isObject()) {
        return fail(QObject::tr("station %1 is not an object").arg(i + 1));
      }
      const auto st = parseStation(stations.at(i).toObject());

      if (st.id.isEmpty() || st.name.isEmpty()) {
        return fail(QObject::tr("station %1 needs a non-empty \"id\" and \"name\"").arg(i + 1));
      }
      if (seen_ids.contains(st.id)) {
        return fail(QObject::tr("duplicate station id \"%1\"").arg(st.id));
      }
      if (!st.isValid()) {
        return fail(QObject::tr("station \"%1\" needs a \"url\" that is http or https: %2")
                      .arg(st.id, st.url.isEmpty() ? QObject::tr("(empty)") : st.url));
      }
      // Non-fatal: Stream::buildRequest sends only url().path(), so a query
      // string is silently dropped on connect. Worth telling the user.
      if (QUrl(st.url).hasQuery()) {
        qWarning() << "radio: station" << st.id << "url has a query string,"
                   << "which is dropped when connecting:" << st.url;
      }

      seen_ids.insert(st.id);
      result._stations << st;
    }

    if (error) {
      error->clear();
    }
    return result;
  }

  void Catalog::layerOver(const Catalog &base) {
    QVector<Station> merged = base._stations;
    for (const auto &st : std::as_const(_stations)) {
      bool replaced = false;
      for (auto &existing : merged) {
        if (existing.id == st.id) {
          existing = st;
          replaced = true;
          break;
        }
      }
      if (!replaced) {
        merged << st;
      }
    }
    _stations = merged;
  }

  QByteArray Catalog::builtinJson() {
    QFile f(kBuiltinResource);
    if (!f.open(QIODevice::ReadOnly)) {
      qWarning() << "radio: cannot open built-in station list" << kBuiltinResource;
      return QByteArray();
    }
    return f.readAll();
  }

  void Catalog::setUserFilePath(const QString &path) {
    if (userPathRef() == path) {
      return;
    }
    userPathRef() = path;
    reload();
  }

  QString Catalog::userFilePath() {
    return userPathRef();
  }

  QString Catalog::lastError() {
    return lastErrorRef();
  }

  void Catalog::reload() {
    delete cacheRef();
    cacheRef() = nullptr;
  }

  const Catalog &Catalog::active() {
    if (cacheRef()) {
      return *cacheRef();
    }

    lastErrorRef().clear();

    QString builtin_error;
    auto *result = new Catalog(fromJson(builtinJson(), &builtin_error));
    if (!builtin_error.isEmpty()) {
      qWarning() << "radio: built-in station list is broken:" << builtin_error;
    }

    const auto path = userPathRef();
    if (!path.isEmpty() && QFile::exists(path)) {
      QFile f(path);
      if (!f.open(QIODevice::ReadOnly)) {
        lastErrorRef() = QObject::tr("Cannot read %1").arg(path);
      } else {
        QString user_error;
        auto user = fromJson(f.readAll(), &user_error);
        if (!user_error.isEmpty()) {
          lastErrorRef() = user_error;
        } else if (user.extendsBuiltin()) {
          user.layerOver(*result);
          *result = user;
        } else {
          *result = user;
        }
      }
    }

    cacheRef() = result;
    return *result;
  }
}
