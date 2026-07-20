#ifndef DIRECTORYDATAMODELRADIO_H
#define DIRECTORYDATAMODELRADIO_H

#include "config/global.h"
#include "directory_ui/directorymodel/radio/item.h"
#include "radio/catalog.h"
#include "track.h"

#include <QAbstractItemModel>
#include <QModelIndexList>
#include <QObject>
#include <QString>

namespace DirectoryUi {
  namespace DirectoryModel {
    namespace RadioRole {
      enum {
        Path = Qt::UserRole,
        Subtitle,
        Description,
        StreamUrl,
        Homepage,
        Logo,
        IsStation
      };
    }

    class Radio : public QAbstractItemModel {
      Q_OBJECT

    signals:
      void directoryLoaded(const QString &path);

    public:
      explicit Radio(Config::Global &global_cfg, QObject *parent = nullptr);
      ~Radio();

      void loadAsync(const QString &path);
      void filter(const QString &term);
      QModelIndex rootIndex() const;
      QString filePath(const QModelIndex &index) const;

      QVector<Track> tracksAt(const QModelIndexList &indexes) const;
      QString displayName(const QModelIndex &index) const;
      bool isStation(const QModelIndex &index) const;

      QModelIndex index(int row, int column, const QModelIndex &parent) const override;
      QModelIndex parent(const QModelIndex &child) const override;
      int rowCount(const QModelIndex &parent) const override;
      int columnCount(const QModelIndex &parent) const override;
      QVariant data(const QModelIndex &index, int role) const override;
      bool hasChildren(const QModelIndex &parent) const override;

    private slots:
      void onLogoAvailable(const QString &station_id);

    private:
      void rebuild();
      RadioItem *itemFromIndex(const QModelIndex &index) const;
      Track trackFor(const RadioItem *item) const;

      Config::Global &global_conf;
      RadioItem *root_item = nullptr;
      QString last_filter_term;
    };
  }
}

#endif // DIRECTORYDATAMODELRADIO_H
