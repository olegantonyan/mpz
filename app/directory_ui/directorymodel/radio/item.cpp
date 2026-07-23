#include "item.h"

namespace DirectoryUi {
  namespace DirectoryModel {
    RadioItem::RadioItem(bool is_grp, const QString &nm, RadioItem *parent_item) :
      name(nm), is_group(is_grp), parent(parent_item) {
    }

    RadioItem::~RadioItem() {
      qDeleteAll(children);
    }

    bool RadioItem::match(const QString &filter) const {
      return filter.isEmpty()
          || name.contains(filter, Qt::CaseInsensitive)
          || subtitle.contains(filter, Qt::CaseInsensitive);
    }

    bool RadioItem::update_visibility(const QString &filter) {
      bool any_child_visible = false;
      for (const auto &item : std::as_const(children)) {
        if (item && item->update_visibility(filter)) {
          any_child_visible = true;
        }
      }
      // A group whose own name matches keeps all its children visible.
      const bool self_matches = match(filter);
      if (is_group && self_matches && !any_child_visible) {
        for (const auto &item : std::as_const(children)) {
          if (item) {
            item->visible = true;
          }
        }
        any_child_visible = !children.isEmpty();
      }
      visible = self_matches || any_child_visible;
      return visible;
    }

    int RadioItem::visible_children_count() const {
      int count = 0;
      for (const auto &item : std::as_const(children)) {
        if (item && item->visible) {
          count++;
        }
      }
      return count;
    }

    int RadioItem::row() const {
      if (!parent) {
        return 0;
      }
      int visible_row = 0;
      for (const auto &item : parent->children) {
        if (item == this) {
          return visible_row;
        }
        if (item->visible) {
          visible_row++;
        }
      }
      return 0;
    }

    RadioItem *RadioItem::child(int row) {
      int visible_row = 0;
      for (const auto &item : std::as_const(children)) {
        if (item && item->visible) {
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
