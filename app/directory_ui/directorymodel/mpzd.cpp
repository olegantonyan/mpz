#include "mpzd.h"
#include "icons.h"

#include <QFileIconProvider>
#include <QFileInfo>

namespace DirectoryUi {
  namespace DirectoryModel {
    Mpzd::Mpzd(MpzdClient::Client &cl, QObject *parent) : QAbstractItemModel{parent}, client(cl) {
      root_item = new TreeItem(true, "");
    }

    Mpzd::~Mpzd() {
      delete root_item;
    }

    void Mpzd::loadAsync(const QString &path) {
      if (client.isConnected() && client.currentUrl() == QUrl(path)) {
        reload();
      } else {
        client.openConnection(QUrl(path));
      }
    }

    void Mpzd::onMpzdReady() {
      reload();
    }

    void Mpzd::onMpzdLost() {
      generation++;
      beginResetModel();
      resetRoot();
      endResetModel();
    }

    void Mpzd::resetRoot() {
      delete root_item;
      root_item = new TreeItem(true, "");
      root_item->loaded = true;
    }

    void Mpzd::reload() {
      const quint64 gen = ++generation;
      beginResetModel();
      resetRoot();
      endResetModel();
      const QString url = client.currentUrl().toString();
      client.request({"libraries"}, this, [this, gen, url](const MpzdClient::Reply &reply) {
        if (gen != generation) {
          return;
        }
        QStringList libraries;
        for (const auto &line : reply.lines) {
          if (line.first == "library") {
            libraries << line.second;
          }
        }
        if (libraries.size() == 1) {
          listInto(root_item, libraries.first(), [this, url]() { emit directoryLoaded(url); });
          return;
        }
        beginResetModel();
        for (const auto &lib : std::as_const(libraries)) {
          auto *item = new TreeItem(true, lib, 0, root_item);
          item->name = lib;
          item->update_visibility(filter_term);
          root_item->children.append(item);
        }
        endResetModel();
        emit directoryLoaded(url);
      });
    }

    void Mpzd::listInto(TreeItem *item, const QString &path, std::function<void()> done) {
      const quint64 gen = generation;
      item->loaded = true;
      client.request({"ls", path, sort_mode}, this, [this, gen, item, path, done](const MpzdClient::Reply &reply) {
        if (gen != generation) {
          return;
        }
        QVector<TreeItem *> fresh;
        for (const auto &line : reply.lines) {
          if (line.first != "dir" && line.first != "file") {
            continue;
          }
          const QString child_path = path.endsWith('/') ? path + line.second : path + "/" + line.second;
          fresh.append(new TreeItem(line.first == "dir", child_path, 0, item));
        }
        if (item == root_item) {
          beginResetModel();
          for (auto *c : std::as_const(fresh)) {
            c->update_visibility(filter_term);
          }
          item->children = fresh;
          endResetModel();
        } else if (!fresh.isEmpty()) {
          const int first = item->visible_children_count();
          beginInsertRows(indexOf(item), first, first + fresh.size() - 1);
          item->children.append(fresh);
          endInsertRows();
        }
        if (done) {
          done();
        }
      });
    }

    void Mpzd::filter(const QString &term) {
      filter_term = term;
      beginResetModel();
      for (auto *i : std::as_const(root_item->children)) {
        i->update_visibility(term);
      }
      endResetModel();
    }

    void Mpzd::sort(int column, Qt::SortOrder order) {
      const QString mode = column == 3 ? (order == Qt::AscendingOrder ? "date" : "-date") : "name";
      if (mode == sort_mode) {
        return;
      }
      sort_mode = mode;
      if (client.isConnected()) {
        reload();
      }
    }

    QModelIndex Mpzd::rootIndex() const {
      return QModelIndex();
    }

    QString Mpzd::filePath(const QModelIndex &index) const {
      auto *item = itemFrom(index);
      return item ? item->path : QString();
    }

    TreeItem *Mpzd::itemFrom(const QModelIndex &index) const {
      return index.isValid() ? static_cast<TreeItem *>(index.internalPointer()) : nullptr;
    }

    QModelIndex Mpzd::indexOf(TreeItem *item) const {
      if (!item || item == root_item || !item->parent) {
        return QModelIndex();
      }
      return createIndex(item->row(), 0, item);
    }

    QModelIndex Mpzd::index(int row, int column, const QModelIndex &parent) const {
      if (!hasIndex(row, column, parent)) {
        return QModelIndex();
      }
      TreeItem *parent_item = parent.isValid() ? itemFrom(parent) : root_item;
      TreeItem *child = parent_item->child(row);
      return child ? createIndex(row, column, child) : QModelIndex();
    }

    QModelIndex Mpzd::parent(const QModelIndex &child) const {
      auto *item = itemFrom(child);
      if (!item || !item->parent || item->parent == root_item) {
        return QModelIndex();
      }
      return indexOf(item->parent);
    }

    int Mpzd::rowCount(const QModelIndex &parent) const {
      TreeItem *item = parent.isValid() ? itemFrom(parent) : root_item;
      return item->visible_children_count();
    }

    int Mpzd::columnCount(const QModelIndex &parent) const {
      Q_UNUSED(parent);
      return 1;
    }

    QVariant Mpzd::data(const QModelIndex &index, int role) const {
      auto *item = itemFrom(index);
      if (!item) {
        return QVariant();
      }
      switch (role) {
      case Qt::DisplayRole:
        return item->name;
      case Qt::UserRole:
        return item->path;
      case Qt::DecorationRole:
        if (item->is_directory) {
          return Icons::get(Icons::Icon::Folder);
        }
        return QFileIconProvider().icon(QFileInfo(item->name));
      default:
        return QVariant();
      }
    }

    bool Mpzd::hasChildren(const QModelIndex &parent) const {
      auto *item = parent.isValid() ? itemFrom(parent) : root_item;
      return item->is_directory && (!item->loaded || !item->children.isEmpty());
    }

    bool Mpzd::canFetchMore(const QModelIndex &parent) const {
      auto *item = itemFrom(parent);
      return item && item->is_directory && !item->loaded;
    }

    void Mpzd::fetchMore(const QModelIndex &parent) {
      auto *item = itemFrom(parent);
      if (!item || !item->is_directory || item->loaded) {
        return;
      }
      listInto(item, item->path);
    }
  }
}
