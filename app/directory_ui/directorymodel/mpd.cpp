#include "mpd.h"

#include <QDebug>
#include <QUrl>
#include <QVector>

namespace DirectoryUi {
  namespace DirectoryModel {
    Mpd::Mpd(QObject *parent) : QAbstractItemModel{parent} {
      connection = nullptr;
      root_item = new TreeItem("Music", "", true);
    }

    Mpd::~Mpd() {
      if (connection) {
        mpd_connection_free(connection);
      }
      delete root_item;
    }

    void Mpd::loadAsync(const QString &path) {
      auto url = QUrl(path);
      qDebug() << "MPD load async" << url.host() << url.port();
      if (connection) {
        mpd_connection_free(connection);
      }
      connection = mpd_connection_new(url.host().toUtf8().constData(), url.port(), 0);
      if (!connection) {
        qWarning() << "error allocation mpd connection";
        return;
      }
      if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
        qWarning() << mpd_connection_get_error_message(connection);
        mpd_connection_free(connection);
        return;
      }

      beginResetModel();
      load_directory(root_item, "");
      endResetModel();
    }

    void Mpd::setNameFilters(const QStringList &filters) {
    }

    QModelIndex Mpd::rootIndex() const {
      return QModelIndex();
    }

    QString Mpd::filePath(const QModelIndex &index) const {
      return "";
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::index(int row, int column, const QModelIndex &parent) const {
      return QModelIndex();
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::parent(const QModelIndex &child) const {
      return QModelIndex();
    }

    int DirectoryUi::DirectoryModel::Mpd::rowCount(const QModelIndex &parent) const {
      return 0;
    }

    int DirectoryUi::DirectoryModel::Mpd::columnCount(const QModelIndex &parent) const {
      return 0;
    }

    QVariant DirectoryUi::DirectoryModel::Mpd::data(const QModelIndex &index, int role) const {
      return QVariant("0");
    }

    void Mpd::load_directory(TreeItem *parent, const QString &path) {
      if (!connection) {
        qWarning() << "mpd connection does not exist";
        return;
      }

      if (!mpd_send_list_meta(connection, path.toUtf8().constData())) {
        qWarning() << "mpd_send_list_all:" << mpd_connection_get_error_message(connection);
        return;
      }

      struct mpd_entity* entity;
      QVector<TreeItem*> new_items;

      while ((entity = mpd_recv_entity(connection)) != nullptr) {
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY) {
          const struct mpd_directory* dir = mpd_entity_get_directory(entity);
          const char* dirPath = mpd_directory_get_path(dir);
          QString fullPath = QString::fromUtf8(dirPath);
          QString dirName = fullPath.split('/').last();

          TreeItem* item = new TreeItem(dirName, fullPath, true, parent);
          new_items.append(item);
        } else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
          const struct mpd_song* song = mpd_entity_get_song(entity);
          const char* songPath = mpd_song_get_uri(song);
          const char* title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);

          QString songName = title ? QString::fromUtf8(title)
                                   : QString::fromUtf8(songPath).split('/').last();
          QString fullPath = QString::fromUtf8(songPath);

          TreeItem* item = new TreeItem(songName, fullPath, false, parent);
          new_items.append(item);
        }

        mpd_entity_free(entity);
      }

      if (!new_items.isEmpty()) {
        int first = parent->children.size();
        int last = first + new_items.size() - 1;

        QModelIndex parentIndex = parent == root_item ? QModelIndex() : createIndexForItem(parent);
        beginInsertRows(parentIndex, first, last);
        parent->children.append(new_items);
        endInsertRows();
      }

      mpd_response_finish(connection);
      parent->loaded = true;

    }

    QModelIndex Mpd::createIndexForItem(TreeItem *item) const {
      if (!item || !item->parent) {
        return QModelIndex();
      }
      int row = item->parent->children.indexOf(item);
      return createIndex(row, 0, item);
    }
  }
}

