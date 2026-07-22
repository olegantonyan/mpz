#include "catalog.h"

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

    Station parseStation(const QJsonObject &o) {
      Station st;
      st.id = o.value("id").toString().trimmed();
      st.name = o.value("name").toString().trimmed();
      st.group = o.value("group").toString().trimmed();
      st.url = o.value("url").toString().trimmed();
      st.codec = o.value("codec").toString().trimmed().toLower();
      st.bitrate = static_cast<quint16>(o.value("bitrate").toInt());
      st.homepage = o.value("homepage").toString().trimmed();
      return st;
    }
  }

  QByteArray Catalog::builtinJson() {
    QFile f(kBuiltinResource);
    if (!f.open(QIODevice::ReadOnly)) {
      qWarning() << "radio: cannot open built-in station list" << kBuiltinResource;
      return QByteArray();
    }
    return f.readAll();
  }

  QVector<Station> Catalog::builtin() {
    QString error;
    auto stations = fromJson(builtinJson(), &error);
    if (!error.isEmpty()) {
      qWarning() << "radio: built-in station list is broken:" << error;
    }
    return stations;
  }

  QVector<Station> Catalog::fromJson(const QByteArray &json, QString *error) {
    const auto fail = [error](const QString &message) {
      if (error) {
        *error = message;
      }
      return QVector<Station>();
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

    QVector<Station> result;
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

      seen_ids.insert(st.id);
      result << st;
    }

    if (error) {
      error->clear();
    }
    return result;
  }

  QStringList Catalog::groups(const QVector<Station> &stations) {
    QStringList result;
    for (const auto &st : stations) {
      if (!st.group.isEmpty() && !result.contains(st.group)) {
        result << st.group;
      }
    }
    return result;
  }
}
