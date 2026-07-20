#ifndef RADIOSTATIONSDIALOG_H
#define RADIOSTATIONSDIALOG_H

#include "radio/station.h"

#include <QDialog>
#include <QVector>

class QTableWidget;

namespace DirectoryUi {
  // Lists the user's radio stations with full add / edit / remove, plus
  // "Restore defaults" (re-seed from the built-in list). The edited list is
  // available via stations() once the dialog is accepted.
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
