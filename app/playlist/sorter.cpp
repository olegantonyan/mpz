#include "sorter.h"

#include <QStringView>

namespace Playlist {
  namespace {
    int leading_number(const QString &s) {
      int digits = 0;
      while (digits < s.size() && s.at(digits).isDigit()) {
        digits++;
      }
      return s.left(digits).toInt();
    }

    int natural_compare(const QString &a, const QString &b) {
      int i = 0;
      int j = 0;
      while (i < a.size() && j < b.size()) {
        if (a.at(i).isDigit() && b.at(j).isDigit()) {
          const int a_start = i;
          const int b_start = j;
          while (i < a.size() && a.at(i).isDigit()) {
            i++;
          }
          while (j < b.size() && b.at(j).isDigit()) {
            j++;
          }
          int a_digits = a_start;
          while (a_digits < i - 1 && a.at(a_digits) == QLatin1Char('0')) {
            a_digits++;
          }
          int b_digits = b_start;
          while (b_digits < j - 1 && b.at(b_digits) == QLatin1Char('0')) {
            b_digits++;
          }
          const QStringView a_number = QStringView(a).mid(a_digits, i - a_digits);
          const QStringView b_number = QStringView(b).mid(b_digits, j - b_digits);
          if (a_number.size() != b_number.size()) {
            return a_number.size() < b_number.size() ? -1 : 1;
          }
          const int digits_cmp = a_number.compare(b_number);
          if (digits_cmp != 0) {
            return digits_cmp;
          }
          if (a_digits - a_start != b_digits - b_start) {
            return (a_digits - a_start) < (b_digits - b_start) ? -1 : 1;
          }
        } else {
          const int a_start = i;
          const int b_start = j;
          while (i < a.size() && !a.at(i).isDigit()) {
            i++;
          }
          while (j < b.size() && !b.at(j).isDigit()) {
            j++;
          }
          const int text_cmp = QString::localeAwareCompare(a.mid(a_start, i - a_start), b.mid(b_start, j - b_start));
          if (text_cmp != 0) {
            return text_cmp;
          }
        }
      }
      if (i < a.size()) {
        return 1;
      }
      if (j < b.size()) {
        return -1;
      }
      return 0;
    }

    int path_compare(const QString &a, const QString &b) {
      const QStringList a_parts = a.split(QLatin1Char('/'));
      const QStringList b_parts = b.split(QLatin1Char('/'));
      const int common = qMin(a_parts.size(), b_parts.size());
      for (int i = 0; i < common; i++) {
        const int cmp = natural_compare(a_parts.at(i), b_parts.at(i));
        if (cmp != 0) {
          return cmp;
        }
      }
      if (a_parts.size() != b_parts.size()) {
        return a_parts.size() < b_parts.size() ? -1 : 1;
      }
      return 0;
    }
  }

  Sorter::Sorter(const QString &c) {
    for (const auto &i : c.split("/")) {
      if (!i.isEmpty()) {
        criteria << i.simplified().toUpper();
      }
    }
    //qDebug() << criteria;
  }

  QString Sorter::defaultCriteria() {
    return "YEAR / ALBUM / DIRECTORY / DISCNUMBER / TRACKNUMBER / FILENAME / TITLE";
  }

  bool Sorter::condition(const Track &t1, const Track &t2) const {
    for (const auto &i : std::as_const(criteria)) {
      int cmp = compare(t1, t2, i);
      if (cmp > 0) {
        return true;
      } else if (cmp < 0) {
        return false;
      }
    }

    return false;
  }

  int Sorter::compare(const Track &t1, const Track &t2, QString attr) const {
    int result = 0;
    int order = 1;

    if (attr.startsWith("-")) {
      attr.remove(0, 1);
      order = -1;
    }

    if (attr == "ARTIST") {
      result = compare_artist(t1, t2);
    } else if (attr == "ALBUMARTIST") {
      result = compare_album_artist(t1, t2);
    } else if (attr == "ALBUM") {
      result = compare_album(t1, t2);
    } else if (attr == "YEAR") {
      result = compare_year(t1, t2);
    } else if (attr == "TRACKNUMBER") {
      result = compare_track_number(t1, t2);
    } else if (attr == "DISCNUMBER") {
      result = compare_disc_number(t1, t2);
    } else if (attr == "FILENAME") {
      result = compare_filename(t1, t2);
    } else if (attr == "TITLE") {
      result = compare_title(t1, t2);
    }else if (attr == "DIRECTORY") {
      result = compare_dir(t1, t2);
    }

    return result * order;
  }

  int Sorter::compare_year(const Track &t1, const Track &t2) const {
    if (t1.year() < t2.year()) {
      return 1;
    } else if (t1.year() > t2.year()) {
      return -1;
    }
    return 0;
  }

  int Sorter::compare_album(const Track &t1, const Track &t2) const {
    return -QString::localeAwareCompare(t1.album(), t2.album());
  }

  int Sorter::compare_track_number(const Track &t1, const Track &t2) const {
    if (t1.track_number() < t2.track_number()) {
      return 1;
    } else if (t1.track_number() > t2.track_number()) {
      return -1;
    }
    return 0;
  }

  int Sorter::compare_disc_number(const Track &t1, const Track &t2) const {
    const int n1 = leading_number(t1.disc_number());
    const int n2 = leading_number(t2.disc_number());
    if (n1 < n2) {
      return 1;
    } else if (n1 > n2) {
      return -1;
    }
    return -QString::localeAwareCompare(t1.disc_number(), t2.disc_number());
  }

  int Sorter::compare_filename(const Track &t1, const Track &t2) const {
    return -natural_compare(t1.filename(), t2.filename());
  }

  int Sorter::compare_title(const Track &t1, const Track &t2) const {
    return -QString::localeAwareCompare(t1.title(), t2.title());
  }

  int Sorter::compare_artist(const Track &t1, const Track &t2) const {
    return -QString::localeAwareCompare(t1.artist(), t2.artist());
  }

  int Sorter::compare_album_artist(const Track &t1, const Track &t2) const {
    return -QString::localeAwareCompare(t1.album_artist(), t2.album_artist());
  }

  int Sorter::compare_dir(const Track &t1, const Track &t2) const {
    return -path_compare(t1.dir(), t2.dir());
  }
}
