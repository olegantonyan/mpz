#ifndef SORTER_H
#define SORTER_H

#include "track.h"

#include <QStringList>
#include <QDebug>

namespace Playlist {
  class Sorter {
  public:
    static QString defaultCriteria();

    explicit Sorter(const QString &criteria = Sorter::defaultCriteria());

  bool condition(const Track &t1, const Track &t2) const;

  private:
    QStringList criteria;

    int compare(const Track &t1, const Track &t2, QString attr) const;
    int compare_year(const Track &t1, const Track &t2) const;
    int compare_album(const Track &t1, const Track &t2) const;
    int compare_track_number(const Track &t1, const Track &t2) const;
    int compare_filename(const Track &t1, const Track &t2) const;
    int compare_title(const Track &t1, const Track &t2) const;
    int compare_artist(const Track &t1, const Track &t2) const;
    int compare_dir(const Track &t1, const Track &t2) const;
  };
}

#endif // SORTER_H
