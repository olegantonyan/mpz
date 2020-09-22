#ifndef PLAYBACKLOGMODEL_H
#define PLAYBACKLOGMODEL_H

#include "config/local.h"

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
    explicit Model(Config::Local &local_c, int max_size, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Item last() const;
    Item itemAt(const QModelIndex& index) const;

    QString itemsToCsv() const;

  signals:
    void changed();
    void totalPlayTimeChanged(int value);
    void thisSessionPlayTimeChanged(int value);

  public slots:
    void append(const Item& item);
    void incrementPlayTime(int by = 1);

  private:
    Config::Local &local_config;
    int max_size;
    QList<Item> items;
    int total_play_time;
    int this_session_play_time;
  };
}

#endif // PLAYBACKLOGMODEL_H
