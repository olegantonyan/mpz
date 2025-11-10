#include "treeitem.h"

#include <QFileInfo>

namespace DirectoryUi {
  namespace DirectoryModel {
    TreeItem::TreeItem(bool is_dir, const QString& pth, time_t last_mod, TreeItem *parent_item) : last_modified(last_mod), is_directory(is_dir), parent(parent_item), visible(true) {
      loaded = false;
      setPathWName(pth);
    }

    TreeItem::~TreeItem() {
      qDeleteAll(children);
    }

    void TreeItem::setPathWName(const QString &pth) {
      path = pth;
      name = QFileInfo(path).fileName();
    }

    bool TreeItem::match(const QString &filter) const {
      return filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
    }

    bool TreeItem::update_visibility(const QString &filter) {
      visible = match(filter);
      return visible;
    }

    int TreeItem::visible_children_count() const {
      int count = 0;
      for (auto item : children) {
        if (item->visible) {
          count++;
        }
      }
      return count;
    }

    int TreeItem::row() const {
      if (!parent) {
        return 0;
      }
      int visible_row = 0;
      for (auto item : parent->children) {
        if (item == this) {
          return visible_row;
        }
        if (item->visible) {
          visible_row++;
        }
      }
      return 0;
    }

    TreeItem *TreeItem::child(int row) {
      int visible_row = 0;
      for (auto item : children) {
        if (item->visible) {
          if (visible_row == row) {
            return item;
          }
          visible_row++;
        }
      }
      return nullptr;
    }
  }
}
