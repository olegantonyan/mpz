#ifndef COLUMNSCONFIG_H
#define COLUMNSCONFIG_H

#include "track.h"
#include "config/value.h"

#include <QString>
#include <QVector>
#include <QDebug>

namespace PlaylistUi {
  class ColumnsConfig {
  public:
    static ColumnsConfig deserialize(const Config::Value &v);

    explicit ColumnsConfig();

    int count() const;
    double width(int col) const;
    bool stretch(int col) const;
    QString field(int col) const;
    Qt::Alignment align(int col) const;

    QString value(int col, const Track& track) const;

    void setWidths(const QVector<double> &arr);
    void setStretches(const QVector<bool> &arr);
    void setFields(const QVector<QString> &arr);
    void setAligns(const QVector<Qt::Alignment> &arr);

    Config::Value serialize() const;

    void vaidate();

  private:
    QVector<double> widths;
    QVector<bool> stretches;
    QVector<QString> fields;
    QVector<Qt::Alignment> aligns;
  };
}

#endif // COLUMNSCONFIG_H
