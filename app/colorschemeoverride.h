#ifndef COLORSCHEMEOVERRIDE_H
#define COLORSCHEMEOVERRIDE_H

#include <QPalette>
#include <QString>

class ColorSchemeOverride {
public:
  enum class Mode { Auto, Light, Dark };

  static Mode modeFromString(const QString &mode);

  void apply(Mode mode);

private:
  void restore();

  QString native_style;
  bool scheme_requested = false;
  bool style_swapped = false;
  bool palette_overridden = false;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QPalette native_palette;
#endif
};

#endif // COLORSCHEMEOVERRIDE_H
