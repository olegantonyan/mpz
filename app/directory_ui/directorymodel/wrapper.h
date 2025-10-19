#ifndef DIRECTORYDATAMODELWRAPPER_H
#define DIRECTORYDATAMODELWRAPPER_H

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
      enum ActiveMode {
        DIRECTORY_MODEL_LOCALFS,
        DIRECTORY_MODEL_MPD
      };

      explicit Wrapper(QObject *parent = nullptr);

      void setActiveMode(enum ActiveMode new_active);

      void loadAsync(const QString &path);

      QAbstractItemModel *model() const;
      QModelIndex rootIndex() const;
      QString rootPath() const;
      QString filePath(const QModelIndex &index) const;
      void setNameFilters(const QStringList &filters);

    signals:
      void directoryLoaded(const QString &path);

    public slots:
      void sortBy(const QString &direction);

    private:
      Localfs *localfs;
#ifdef ENABLE_MPD_SUPPORT
      Mpd *mpd;
#endif
      enum ActiveMode active;
    };
  }
}

#endif // DIRECTORYDATAMODELWRAPPER_H
