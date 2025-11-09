#include "mpd.h"

#include <QDebug>
#include <QUrl>
#include <QVector>
#include <QVariant>
#include <QApplication>
#include <QStyle>

namespace DirectoryUi {
  namespace DirectoryModel {
    Mpd::Mpd(QObject *parent) : QAbstractItemModel{parent} {
      connection = nullptr;
      root_item = new TreeItem("", "", true);
    }

    Mpd::~Mpd() {
      if (connection) {
        mpd_connection_free(connection);
      }
      delete root_item;
    }

    void Mpd::loadAsync(const QString &path) {
      delete root_item;
      root_item = new TreeItem("", "", true);

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
      sort(0, Qt::AscendingOrder);

      emit directoryLoaded(path);
    }

    void Mpd::setNameFilters(const QStringList &filters) {
    }

    QModelIndex Mpd::rootIndex() const {
      if (!root_item) {
        return QModelIndex();
      }
      return createIndexForItem(root_item);
    }

    QString Mpd::filePath(const QModelIndex &index) const {
      return "";
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::index(int row, int column, const QModelIndex &parent) const {
      if (!hasIndex(row, column, parent)) {
        return QModelIndex();
      }

      TreeItem* parentItem = parent.isValid() ? static_cast<TreeItem*>(parent.internalPointer()) : root_item;

      if (row < parentItem->children.size()) {
        return createIndex(row, column, parentItem->children[row]);
      }
      return QModelIndex();
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::parent(const QModelIndex &child) const {
      if (!child.isValid()) {
        return QModelIndex();
      }

      TreeItem* childItem = static_cast<TreeItem*>(child.internalPointer());
      TreeItem* parentItem = childItem->parent;

      if (!parentItem || parentItem == root_item) {
          return QModelIndex();
      }
      TreeItem* grandparent = parentItem->parent;
      if (!grandparent) {
          return QModelIndex();
      }
      int row = grandparent->children.indexOf(parentItem);
      return createIndex(row, 0, parentItem);
    }

    int DirectoryUi::DirectoryModel::Mpd::rowCount(const QModelIndex &parent) const {
      TreeItem* parent_item = parent.isValid() ? static_cast<TreeItem*>(parent.internalPointer()) : root_item;
      return parent_item->children.size();
    }

    int DirectoryUi::DirectoryModel::Mpd::columnCount(const QModelIndex &parent) const {
      Q_UNUSED(parent);
      return 1;
    }

    QVariant DirectoryUi::DirectoryModel::Mpd::data(const QModelIndex &index, int role) const {
      if (!index.isValid()) {
        return QVariant();
      }

      TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

      switch (role) {
      case Qt::DisplayRole:
        return item->name;
      case Qt::UserRole:
        return item->path;
      case Qt::DecorationRole:
        if (item->isDirectory) {
          return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        } else {
          return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
        }
      }

      return QVariant();
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
          item->last_modified = mpd_directory_get_last_modified(dir);
          new_items.append(item);
        } else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
          const struct mpd_song* song = mpd_entity_get_song(entity);
          const char* songPath = mpd_song_get_uri(song);
          //const char* title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);

          //QString songName = title ? QString::fromUtf8(title) : QString::fromUtf8(songPath).split('/').last();
          QString fullPath = QString::fromUtf8(songPath);
          QString filename = QString::fromUtf8(songPath).split('/').last();

          TreeItem* item = new TreeItem(filename, fullPath, false, parent);
          item->last_modified = mpd_song_get_last_modified(song);
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

    bool Mpd::hasChildren(const QModelIndex &parent) const {
      if (parent.isValid()) {
        TreeItem* item = static_cast<TreeItem*>(parent.internalPointer());
        return item->isDirectory;
      }
      return root_item->isDirectory;
    }

    bool Mpd::canFetchMore(const QModelIndex &parent) const {
      if (!parent.isValid()) {
        return false;
      }
      TreeItem* item = static_cast<TreeItem*>(parent.internalPointer());
      return item->isDirectory && !item->loaded;
    }

    void Mpd::fetchMore(const QModelIndex &parent) {
      if (!parent.isValid() || !connection) {
        return;
      }
      TreeItem* item = static_cast<TreeItem*>(parent.internalPointer());
      if (!item->isDirectory || item->loaded) {
        return;
      }
      load_directory(item, item->path);
      item->loaded = true;
    }

    void Mpd::sort(int column, Qt::SortOrder order) {
       std::sort(root_item->children.begin(), root_item->children.end(),
         [column, order](const TreeItem *a, const TreeItem *b) {
             switch (column) {
             case 0: {
               if (a->isDirectory && !b->isDirectory) {
                 return true;
               } else if (!a->isDirectory && b->isDirectory) {
                 return false;
               }
               if (order == Qt::AscendingOrder) {
                 return a->name < b->name;
               } else {
                 return a->name > b->name;
               }
             }
             case 3: {
               if (order == Qt::AscendingOrder) {
                 return a->last_modified < b->last_modified;
               } else {
                 return a->last_modified > b->last_modified;
               }
             }

             default:
                return false;
             }
         });

       emit layoutChanged();
    }
  }
}
