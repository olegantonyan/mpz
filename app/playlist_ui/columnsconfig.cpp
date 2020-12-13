#include "columnsconfig.h"

namespace PlaylistUi {
  ColumnsConfig::ColumnsConfig() {
    widths.append(0.28);
    widths.append(0.28);
    widths.append(0.28);
    widths.append(0.05);
    widths.append(0);

    stretches.append(false);
    stretches.append(false);
    stretches.append(false);
    stretches.append(false);
    stretches.append(true);

    fields.append("artist");
    fields.append("album");
    fields.append("title");
    fields.append("year");
    fields.append("length");

    aligns.append(Qt::AlignVCenter);
    aligns.append(Qt::AlignVCenter);
    aligns.append(Qt::AlignVCenter);
    aligns.append(Qt::AlignVCenter | Qt::AlignRight);
    aligns.append(Qt::AlignVCenter | Qt::AlignRight);
  }

  ColumnsConfig ColumnsConfig::deserialize(const Config::Value &v) {
    QVector<double> widths;
    QVector<bool> stretches;
    QVector<QString> fields;
    QVector<Qt::Alignment> aligns;

    for (auto i : v.get<QList<Config::Value>>()) {
      auto map = i.get<QMap<QString, Config::Value>>();

      widths << map["width_percent"].get<int>() / 100.0;
      stretches << map["stretch"].get<bool>();
      fields << map["field"].get<QString>();
      aligns << (map["align"].get<QString>() == "right" ? (Qt::AlignVCenter | Qt::AlignRight) : Qt::AlignVCenter);
    }

    ColumnsConfig result;
    result.setWidths(widths);
    result.setStretches(stretches);
    result.setFields(fields);
    result.setAligns(aligns);
    return result;
  }

  Config::Value ColumnsConfig::serialize() const {
    QList<Config::Value> r;
    for (int i = 0; i < widths.size(); i++) {
      QMap<QString, Config::Value> v;
      v["width_percent"] = static_cast<int>(widths.at(i) * 100);
      v["align"] = QString((aligns.at(i) & Qt::AlignRight) == Qt::AlignRight ? "right" : "left");
      v["stretch"] = stretches.at(i);
      v["field"] = fields.at(i);
      r << v;
    }
    return r;
  }

  int ColumnsConfig::count() const {
    return widths.size();
  }

  double ColumnsConfig::width(int col) const {
    return widths.at(col - 1);
  }

  bool ColumnsConfig::stretch(int col) const {
    return stretches.at(col - 1);
  }

  QString ColumnsConfig::field(int col) const {
    return fields.at(col - 1);
  }

  Qt::Alignment ColumnsConfig::align(int col) const {
    return aligns.at(col - 1);
  }

  QString ColumnsConfig::value(int col, const Track &track) const {
    auto fld = field(col);
    if (fld == "artist") {
      return track.artist();
    } else if (fld == "album") {
      return track.album();
    } else if (fld == "title") {
      return track.title();
    } else if (fld == "year" && track.year() > 0) {
      return QString::number(track.year());
    } else if (fld == "length") {
      return track.formattedDuration();
    }
    return QString();
  }

  void ColumnsConfig::setWidths(const QVector<double> &arr) {
    widths.clear();
    for (auto i : arr) {
      widths.append(i);
    }
  }

  void ColumnsConfig::setStretches(const QVector<bool> &arr) {
    stretches.clear();
    for (auto i : arr) {
      stretches.append(i);
    }
  }

  void ColumnsConfig::setFields(const QVector<QString> &arr) {
    fields.clear();
    for (auto i : arr) {
      fields.append(i);
    }
  }

  void ColumnsConfig::setAligns(const QVector<Qt::Alignment> &arr) {
    aligns.clear();
    for (auto i : arr) {
      aligns.append(i);
    }
  }
}
