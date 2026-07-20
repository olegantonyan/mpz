#ifndef RADIO_ITEM_H
#define RADIO_ITEM_H

#include <QString>
#include <QStringList>
#include <QVector>

namespace DirectoryUi {
  namespace DirectoryModel {
    class RadioItem {
    public:
      RadioItem(bool is_grp, const QString &nm, RadioItem *parent_item = nullptr);
      ~RadioItem();

      RadioItem(const RadioItem &) = delete;
      RadioItem &operator=(const RadioItem &) = delete;

      bool match(const QString &filter) const;
      bool update_visibility(const QString &filter);
      int visible_children_count() const;
      int row() const;
      RadioItem *child(int row);

      QString name;
      QString subtitle;
      QString description;
      QString station_id;
      QString stream_url;
      QString homepage;
      QString logo_url;

      bool is_group;
      RadioItem *parent;
      QVector<RadioItem *> children;
      bool visible = true;
    };
  }
}

#endif // RADIO_ITEM_H
