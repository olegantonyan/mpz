#include "mpd.h"

#include <QDebug>
#include <QUrl>
#include <QVector>
#include <QVariant>
#include <QApplication>
#include <QStyle>
#include <QFileInfo>
#include <QFileIconProvider>

namespace DirectoryUi {
  namespace DirectoryModel {
    Mpd::Mpd(QObject *parent) : QAbstractItemModel{parent} {
      connection = nullptr;
      root_item = new TreeItem(true, "");
    }

    Mpd::~Mpd() {
      if (connection) {
        mpd_connection_free(connection);
      }
      delete root_item;
    }

    void Mpd::loadAsync(const QString &path) {
      delete root_item;
      root_item = new TreeItem(true, "");

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

      TreeItem* parentItem = parent.isValid() ? tree_item_from_index(parent) : root_item;

      if (row < parentItem->children.size()) {
        return createIndex(row, column, parentItem->children[row]);
      }
      return QModelIndex();
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::parent(const QModelIndex &child) const {
      if (!child.isValid()) {
        return QModelIndex();
      }

      TreeItem* childItem = tree_item_from_index(child);
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
      TreeItem* parent_item = parent.isValid() ? tree_item_from_index(parent) : root_item;
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

      TreeItem* item = tree_item_from_index(index);

      switch (role) {
      case Qt::DisplayRole:
        return item->name;
      case Qt::UserRole:
        return item->path;
      case Qt::DecorationRole:
        if (item->is_directory) {
          return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        } else {
          return QFileIconProvider().icon(QFileInfo(item->name));
          //return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
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
          auto item = new TreeItem(
            true,
            QString::fromUtf8(mpd_directory_get_path(dir)),
            mpd_directory_get_last_modified(dir),
            parent
          );
          new_items.append(item);
        } else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
          const struct mpd_song* song = mpd_entity_get_song(entity);
          auto item = new TreeItem(
            false,
            QString::fromUtf8(mpd_song_get_uri(song)),
            mpd_song_get_last_modified(song),
            parent
          );
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
        TreeItem* item = tree_item_from_index(parent);
        return item->is_directory;
      }
      return root_item->is_directory;
    }

    bool Mpd::canFetchMore(const QModelIndex &parent) const {
      if (!parent.isValid()) {
        return false;
      }
      TreeItem* item = tree_item_from_index(parent);
      return item->is_directory && !item->loaded;
    }

    void Mpd::fetchMore(const QModelIndex &parent) {
      if (!parent.isValid() || !connection) {
        return;
      }
      TreeItem* item = tree_item_from_index(parent);
      if (!item->is_directory || item->loaded) {
        return;
      }
      load_directory(item, item->path);
      item->loaded = true;
    }

    TreeItem *Mpd::tree_item_from_index(const QModelIndex &index) const {
      return static_cast<TreeItem*>(index.internalPointer());
    }

    void Mpd::sort(int column, Qt::SortOrder order) {
       std::sort(root_item->children.begin(), root_item->children.end(),
         [column, order](const TreeItem *a, const TreeItem *b) {
             switch (column) {
             case 0: {
               if (a->is_directory && !b->is_directory) {
                 return true;
               } else if (!a->is_directory && b->is_directory) {
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
