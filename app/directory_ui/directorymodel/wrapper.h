#ifndef DIRECTORYDATAMODELWRAPPER_H
#define DIRECTORYDATAMODELWRAPPER_H

#include "modusoperandi.h"
#include "localfs.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "mpd.h"
#endif

#include <QObject>
#include <QString>
#include <QAbstractItemModel>

namespace DirectoryUi {
  namespace DirectoryModel {
    class Wrapper : public QObject {
      Q_OBJECT

    public:
      explicit Wrapper(ModusOperandi &modus, QObject *parent = nullptr);

      void loadAsync(const QString &path);

      QAbstractItemModel *model() const;
      QModelIndex rootIndex() const;
      QString filePath(const QModelIndex &index) const;
      void filter(const QString &filter);

    signals:
      void directoryLoaded(const QString &path);

    public slots:
      void sortBy(const QString &direction);

    private:
      ModusOperandi &modus_operandi;
      Localfs *localfs;
#ifdef ENABLE_MPD_SUPPORT
      Mpd *mpd;
#endif
    };
  }
}

#endif // DIRECTORYDATAMODELWRAPPER_H
