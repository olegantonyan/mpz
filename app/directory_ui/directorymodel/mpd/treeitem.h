#ifndef MPD_TREEITEM_H
#define MPD_TREEITEM_H

#include <QString>
#include <QVector>
#include <time.h>

namespace DirectoryUi {
  namespace DirectoryModel {
    class TreeItem {
    public:
      TreeItem(bool is_dir, const QString& path, time_t last_mod = 0, TreeItem *parent_item = nullptr);
      ~TreeItem();

      void setPathWName(const QString &path);

      QString name;
      QString path;
      bool is_directory;
      time_t last_modified;
      TreeItem* parent;
      QVector<TreeItem*> children;
      bool loaded;
    };
  }
}

#endif // MPD_TREEITEM_H
