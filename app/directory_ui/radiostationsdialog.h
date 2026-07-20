#ifndef RADIOSTATIONSDIALOG_H
#define RADIOSTATIONSDIALOG_H

#include "radio/station.h"

#include <QDialog>
#include <QVector>

class QTableWidget;

namespace DirectoryUi {
  class RadioStationsDialog : public QDialog {
    Q_OBJECT
  public:
    explicit RadioStationsDialog(const QVector<Radio::Station> &stations, QWidget *parent = nullptr);

    QVector<Radio::Station> stations() const;

  private slots:
    void addStation();
    void editStation();
    void removeStation();
    void restoreDefaults();

  private:
    void refreshTable();
    int currentRow() const;
    bool hasId(const QString &id, int except_row) const;

    QVector<Radio::Station> _stations;
    QTableWidget *table;
  };
}

#endif // RADIOSTATIONSDIALOG_H
