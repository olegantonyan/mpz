#include "treeitem.h"

#include <QFileInfo>

namespace DirectoryUi {
  namespace DirectoryModel {
    TreeItem::TreeItem(bool is_dir, const QString& pth, time_t last_mod, TreeItem *parent_item) : last_modified(last_mod), is_directory(is_dir), parent(parent_item) {
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
  }
}
