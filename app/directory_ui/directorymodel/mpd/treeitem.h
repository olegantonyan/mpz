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

      void set_path_with_name(const QString &path);
      bool match(const QString &filter) const;
      bool update_visibility(const QString &filter);
      int visible_children_count() const;
      int row() const;
      TreeItem *child(int row);

      QString name;
      QString path;
      bool is_directory;
      time_t last_modified;
      TreeItem* parent;
      QVector<TreeItem*> children;
      bool loaded;
      bool visible;
    };
  }
}

#endif // MPD_TREEITEM_H
