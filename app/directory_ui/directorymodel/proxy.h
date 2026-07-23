#ifndef DIRECTORYDATAMODELPROXY_H
#define DIRECTORYDATAMODELPROXY_H

#include "modusoperandi.h"
#include "localfs.h"
#include "radio.h"
#include "config/global.h"
#include "track.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "mpd.h"
#endif

#include <QObject>
#include <QString>
#include <QAbstractItemModel>
#include <QModelIndexList>
#include <QSortFilterProxyModel>
#include <QVector>

namespace DirectoryUi {
  namespace DirectoryModel {
    class Proxy: public QSortFilterProxyModel {
      Q_OBJECT

    public:
      explicit Proxy(ModusOperandi &modus, Config::Global &global_cfg, QObject *parent = nullptr);

      void loadAsync(const QString &path);

      QModelIndex rootIndex() const;
      QString filePath(const QModelIndex &index) const;
      void filter(const QString &filter);

      QVector<Track> tracksAt(const QModelIndexList &indexes) const;
      QString displayName(const QModelIndex &index) const;
      bool isStation(const QModelIndex &index) const;

      // Radio is a view state of the library tree, orthogonal to the app mode:
      // the underlying ModusOperandi stays LOCALFS while radio is shown.
      void setRadioActive(bool active);
      bool isRadioActive() const { return radio_active; }

      ModusOperandi &modus_operandi;

    signals:
      void directoryLoaded(const QString &path);

    public slots:
      void sortBy(const QString &direction);
      void switchTo(ModusOperandi::ActiveMode new_mode);

    protected:
      bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

    private:
      Localfs *localfs;
      Radio *radio;
#ifdef ENABLE_MPD_SUPPORT
      Mpd *mpd;
      bool loadAsyncMpdOnce = true;
#endif
      Config::Global &global_conf;
      QString filter_term;
      bool radio_active = false;
    };
  }
}

#endif // DIRECTORYDATAMODELPROXY_H
