#include "mpd.h"

#include <QDebug>
#include <QUrl>
#include <QVector>
#include <QVariant>
#include <QApplication>
#include <QStyle>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QtConcurrent>

namespace DirectoryUi {
  namespace DirectoryModel {
    Mpd::Mpd(MpdConnection &conn, QObject *parent) : QAbstractItemModel{parent}, root_item(nullptr), connection(conn) {
      createRootItem();
      last_sort_column = 0;
      last_sort_order = Qt::AscendingOrder;
      connect(&conn, &MpdConnection::databaseUpdated, this, &Mpd::onDatabaseUpdated);
    }

    Mpd::~Mpd() {
      if (root_item) {
        delete root_item;
      }
    }

    TreeItem *Mpd::createRootItem() {
      QMutexLocker locker(&loading_mutex);
      if (root_item) {
        delete root_item;
      }
      root_item = new TreeItem(true, "");
      return root_item;
    }

    void Mpd::onDatabaseUpdated() {
      QMutexLocker locker(&loading_mutex);
      beginResetModel();
      loadDirectory(createRootItem(), "");
      endResetModel();
      sort(last_sort_column, last_sort_order);
      filter(last_filter_term);
    }

    void Mpd::loadAsync(const QString &path) {
      if (connection.currentUrl().isEmpty()) {
        // only on initial load
        connection.establish(QUrl(path));
      }
    }

    void Mpd::onMpdReady() {
      (void)QtConcurrent::run(QThreadPool::globalInstance(), [=]() {
        connection.waitConnected();
        onDatabaseUpdated();
        emit directoryLoaded(connection.currentUrl().toString());
      });
    }

    void Mpd::filter(const QString &term) {
      beginResetModel();
      for (auto i : root_item->children) {
        i->update_visibility(term);
      }
      endResetModel();
      last_filter_term = term;
    }

    QModelIndex Mpd::rootIndex() const {
      if (!root_item) {
        return QModelIndex();
      }
      return createIndexForItem(root_item);
    }

    QString Mpd::filePath(const QModelIndex &index) const {
      if (!index.isValid()) {
        return "";
      }
      auto i = treeItemFromIndex(index);
      if (!i) {
        return "";
      }
      return i->path;
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::index(int row, int column, const QModelIndex &parent) const {
      if (!hasIndex(row, column, parent)) {
        return QModelIndex();
      }

      TreeItem* parent_item = parent.isValid() ? treeItemFromIndex(parent) : root_item;
      TreeItem *child_item = parent_item->child(row);

      return child_item ? createIndex(row, column, child_item) : QModelIndex();
    }

    QModelIndex DirectoryUi::DirectoryModel::Mpd::parent(const QModelIndex &child) const {
      if (!child.isValid()) {
        return QModelIndex();
      }

      TreeItem *child_item = treeItemFromIndex(child);
      TreeItem *parent_item = child_item->parent;

      if (!parent_item || parent_item == root_item) {
        return QModelIndex();
      }
      return createIndex(parent_item->row(), 0, parent_item);
    }

    int DirectoryUi::DirectoryModel::Mpd::rowCount(const QModelIndex &parent) const {
      TreeItem *parent_item = parent.isValid() ? treeItemFromIndex(parent) : root_item;
      return parent_item->visible_children_count();
    }

    int DirectoryUi::DirectoryModel::Mpd::columnCount(const QModelIndex &parent) const {
      Q_UNUSED(parent);
      return 1;
    }

    QVariant DirectoryUi::DirectoryModel::Mpd::data(const QModelIndex &index, int role) const {
      QVariant none;

      if (!index.isValid()) {
        return none;
      }

      TreeItem* item = treeItemFromIndex(index);

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
        }
      }

      return none;
    }

    void Mpd::loadDirectory(TreeItem *parent, const QString &path) {
      MpdConnectionLocker locker(connection);

      if (!connection.ping()) {
        qWarning() << "mpd connection does not exist";
        return;
      }

      if (!mpd_send_list_meta(connection.conn, path.toUtf8().constData())) {
        qWarning() << "mpd_send_list_all:" << connection.lastError();
        return;
      }

      struct mpd_entity* entity;
      QVector<TreeItem*> new_items;
      while ((entity = mpd_recv_entity(connection.conn)) != nullptr) {
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

      mpd_response_finish(connection.conn);
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
        TreeItem* item = treeItemFromIndex(parent);
        return item->is_directory;
      }
      return root_item->is_directory;
    }

    bool Mpd::canFetchMore(const QModelIndex &parent) const {
      if (!parent.isValid()) {
        return false;
      }
      TreeItem* item = treeItemFromIndex(parent);
      return item->is_directory && !item->loaded;
    }

    void Mpd::fetchMore(const QModelIndex &parent) {
      if (!parent.isValid() || !connection.conn) {
        return;
      }
      TreeItem* item = treeItemFromIndex(parent);
      if (!item->is_directory || item->loaded) {
        return;
      }
      loadDirectory(item, item->path);
      item->loaded = true;
    }

    TreeItem *Mpd::treeItemFromIndex(const QModelIndex &index) const {
      return static_cast<TreeItem*>(index.internalPointer());
    }

    void Mpd::sort(int column, Qt::SortOrder order) {
      std::sort(root_item->children.begin(), root_item->children.end(), [column, order](const TreeItem *a, const TreeItem *b) {
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

      last_sort_column = column;
      last_sort_order = order;

      emit layoutChanged();
    }
  }
}
