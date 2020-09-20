#ifndef MODEL_H
#define MODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QList>
#include <QDebug>

namespace PlaybackLogUi {
  class Item {
  public:
    explicit Item(quint64 track_uid, const QString &text) {
      this->track_uid = track_uid;
      this->text = text;
      this->time = QDateTime::currentDateTime();
    }
    QString text;
    quint64 track_uid;
    QDateTime time;
  };
}

namespace PlaybackLogUi {
  class Model : public QAbstractTableModel {
    Q_OBJECT
  public:
    explicit Model(int max_size, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Item last() const;
    Item itemAt(const QModelIndex& index) const;

  signals:
    void changed();

  public slots:
    void append(const Item& item);

  private:
    int max_size;
    QList<Item> items;
  };
}

#endif // MODEL_H
