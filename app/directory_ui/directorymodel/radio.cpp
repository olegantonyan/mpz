#include "radio.h"

#include "directory_ui/radiolibrary.h"
#include "icons.h"
#include "radio/catalog.h"

#include <QHash>
#include <QUrl>

namespace DirectoryUi {
  namespace DirectoryModel {

    Radio::Radio(Config::Global &global_cfg, QObject *parent) :
      QAbstractItemModel(parent), global_conf(global_cfg) {
      root_item = new RadioItem(true, QString());
    }

    Radio::~Radio() {
      delete root_item;
    }

    void Radio::loadAsync(const QString &path) {
      beginResetModel();
      rebuild();
      endResetModel();
      emit directoryLoaded(path.isEmpty() ? radioLibraryPath() : path);
    }

    void Radio::rebuild() {
      delete root_item;
      root_item = new RadioItem(true, QString());

      auto stations = global_conf.radioStations();
      if (stations.isEmpty()) {
        stations = ::Radio::Catalog::builtin();
      }

      QHash<QString, RadioItem *> group_items;

      for (const auto &st : stations) {
        RadioItem *parent_item = root_item;
        if (!st.group.isEmpty()) {
          auto &group = group_items[st.group];
          if (!group) {
            group = new RadioItem(true, st.group, root_item);
            root_item->children << group;
          }
          parent_item = group;
        }

        auto *item = new RadioItem(false, st.name, parent_item);
        item->subtitle = st.subtitle();
        item->station_id = st.id;
        item->stream_url = st.url;
        item->homepage = st.homepage;
        parent_item->children << item;
      }

      if (!last_filter_term.isEmpty()) {
        for (const auto &i : std::as_const(root_item->children)) {
          i->update_visibility(last_filter_term);
        }
      }
    }

    void Radio::filter(const QString &term) {
      beginResetModel();
      for (const auto &i : std::as_const(root_item->children)) {
        i->update_visibility(term);
      }
      endResetModel();
      last_filter_term = term;
    }

    QModelIndex Radio::rootIndex() const {
      return QModelIndex();
    }

    RadioItem *Radio::itemFromIndex(const QModelIndex &index) const {
      if (!index.isValid()) {
        return root_item;
      }
      return static_cast<RadioItem *>(index.internalPointer());
    }

    QString Radio::filePath(const QModelIndex &index) const {
      auto *item = itemFromIndex(index);
      if (!item || item == root_item) {
        return QString();
      }
      if (item->is_group) {
        return radioLibraryPath() + QStringLiteral("group/") + item->name;
      }
      return radioLibraryPath() + item->station_id;
    }

    bool Radio::isStation(const QModelIndex &index) const {
      auto *item = itemFromIndex(index);
      return item && item != root_item && !item->is_group;
    }

    QString Radio::displayName(const QModelIndex &index) const {
      auto *item = itemFromIndex(index);
      if (!item || item == root_item) {
        return QString();
      }
      if (!item->is_group && item->parent && item->parent != root_item) {
        return item->parent->name + QStringLiteral(" ∕ ") + item->name;
      }
      return item->name;
    }

    Track Radio::trackFor(const RadioItem *item) const {
      QString title = item->name;
      if (item->parent && item->parent != root_item) {
        title = item->parent->name + QStringLiteral(" ∕ ") + item->name;
      }
      return Track(QUrl(item->stream_url), radioLibraryPath() + item->station_id, title);
    }

    QVector<Track> Radio::tracksAt(const QModelIndexList &indexes) const {
      QVector<Track> result;
      for (const auto &index : indexes) {
        auto *item = itemFromIndex(index);
        if (!item || item == root_item) {
          continue;
        }
        if (item->is_group) {
          for (const auto &child : std::as_const(item->children)) {
            if (child && child->visible && !child->is_group) {
              result << trackFor(child);
            }
          }
        } else {
          result << trackFor(item);
        }
      }
      return result;
    }

    QModelIndex Radio::index(int row, int column, const QModelIndex &parent) const {
      if (!hasIndex(row, column, parent)) {
        return QModelIndex();
      }
      RadioItem *parent_item = itemFromIndex(parent);
      RadioItem *child_item = parent_item ? parent_item->child(row) : nullptr;
      return child_item ? createIndex(row, column, child_item) : QModelIndex();
    }

    QModelIndex Radio::parent(const QModelIndex &child) const {
      if (!child.isValid()) {
        return QModelIndex();
      }
      RadioItem *child_item = itemFromIndex(child);
      RadioItem *parent_item = child_item ? child_item->parent : nullptr;
      if (!parent_item || parent_item == root_item) {
        return QModelIndex();
      }
      return createIndex(parent_item->row(), 0, parent_item);
    }

    int Radio::rowCount(const QModelIndex &parent) const {
      if (parent.column() > 0) {
        return 0;
      }
      RadioItem *parent_item = itemFromIndex(parent);
      return parent_item ? parent_item->visible_children_count() : 0;
    }

    int Radio::columnCount(const QModelIndex &parent) const {
      Q_UNUSED(parent);
      return 1;
    }

    bool Radio::hasChildren(const QModelIndex &parent) const {
      RadioItem *item = itemFromIndex(parent);
      return item && item->visible_children_count() > 0;
    }

    QVariant Radio::data(const QModelIndex &index, int role) const {
      QVariant none;
      auto *item = itemFromIndex(index);
      if (!item || item == root_item) {
        return none;
      }

      switch (role) {
      case Qt::DisplayRole: {
        if (!item->is_group) {
          return item->name;
        }
        const int count = item->visible_children_count();
        return count == 1
                 ? tr("%1  ·  1 station").arg(item->name)
                 : tr("%1  ·  %2 stations").arg(item->name).arg(count);
      }
      case Qt::ToolTipRole:
        return item->is_group ? none : QVariant(item->stream_url);
      case Qt::DecorationRole:
        return item->is_group ? Icons::get(Icons::Icon::Folder) : QVariant();
      case RadioRole::Path:
        return filePath(index);
      case RadioRole::Subtitle:
        return item->subtitle;
      case RadioRole::StreamUrl:
        return item->stream_url;
      case RadioRole::Homepage:
        return item->homepage;
      case RadioRole::IsStation:
        return !item->is_group;
      default:
        return none;
      }
    }
  }
}
