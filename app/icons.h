#ifndef ICONS_H
#define ICONS_H

#include <QIcon>
#include <QPixmap>
#include <QSize>

namespace Icons {
  enum class Icon {
    Play, Pause, Stop, Next, Prev,
    Volume, VolumeMuted, Headphones, Settings, Menu, Sort,
    Trash, Folder, FolderReveal, Reload, Save, Info, Details, Help, Cancel
  };

  QIcon get(Icon icon);
  QPixmap pixmap(Icon icon, const QSize &size, QIcon::Mode mode = QIcon::Normal);
}

#endif // ICONS_H
