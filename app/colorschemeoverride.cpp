#include "colorschemeoverride.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QHash>
#include <QStandardPaths>
#include <QStyle>
#include <QStyleHints>
#include <QTextStream>
#include <QVariant>

#include <optional>

namespace {
  constexpr char kKdeColorSchemeProperty[] = "KDE_COLOR_SCHEME_PATH";

#ifndef Q_OS_MACOS
  // Mirrors Qt's private qt_fusionPalette(), which picks light/dark from the platform theme rather than from an argument
  QPalette fusionPalette(bool dark) {
    const QColor window_text = dark ? QColor(240, 240, 240) : QColor(Qt::black);
    const QColor window = dark ? QColor(50, 50, 50) : QColor(239, 239, 239);
    const QColor light = window.lighter(150);
    const QColor mid = window.darker(130);
    const QColor midlight = mid.lighter(110);
    const QColor base = dark ? window.darker(140) : QColor(Qt::white);
    const QColor shade = window.darker(150);
    const QColor shade_disabled = QColor(209, 209, 209).darker(110);
    const QColor text = dark ? window_text : QColor(Qt::black);
    const QColor highlight(48, 140, 198);
    const QColor highlighted_text = dark ? window_text : QColor(Qt::white);
    const QColor text_disabled = dark ? QColor(130, 130, 130) : QColor(190, 190, 190);
    const QColor shadow = shade.darker(135);
    const QColor highlight_disabled(145, 145, 145);
    QColor placeholder = text;
    placeholder.setAlpha(128);

    QPalette palette(window_text, window, light, shade, mid, text, base);
    palette.setBrush(QPalette::Midlight, midlight);
    palette.setBrush(QPalette::Button, window);
    palette.setBrush(QPalette::Shadow, shadow);
    palette.setBrush(QPalette::HighlightedText, highlighted_text);

    palette.setBrush(QPalette::Disabled, QPalette::Text, text_disabled);
    palette.setBrush(QPalette::Disabled, QPalette::WindowText, text_disabled);
    palette.setBrush(QPalette::Disabled, QPalette::ButtonText, text_disabled);
    palette.setBrush(QPalette::Disabled, QPalette::Base, window);
    palette.setBrush(QPalette::Disabled, QPalette::Dark, shade_disabled);
    palette.setBrush(QPalette::Disabled, QPalette::Shadow, shadow.lighter(150));

    palette.setBrush(QPalette::Active, QPalette::Highlight, highlight);
    palette.setBrush(QPalette::Inactive, QPalette::Highlight, highlight);
    palette.setBrush(QPalette::Disabled, QPalette::Highlight, highlight_disabled);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    palette.setBrush(QPalette::Active, QPalette::Accent, highlight);
    palette.setBrush(QPalette::Inactive, QPalette::Accent, highlight);
    palette.setBrush(QPalette::Disabled, QPalette::Accent, highlight_disabled);
#endif

    palette.setBrush(QPalette::PlaceholderText, placeholder);
    if (dark) {
      palette.setBrush(QPalette::Link, highlight);
    }
    return palette;
  }

  QColor mix(const QColor &from, const QColor &to, qreal amount) {
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount,
                            from.greenF() + (to.greenF() - from.greenF()) * amount,
                            from.blueF() + (to.blueF() - from.blueF()) * amount);
  }

  QHash<QString, QColor> readKdeColorScheme(const QString &path) {
    QHash<QString, QColor> colors;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      return colors;
    }
    QTextStream in(&file);
    QString group;
    while (!in.atEnd()) {
      const QString line = in.readLine().trimmed();
      if (line.startsWith(QLatin1Char('['))) {
        group = line;
        continue;
      }
      const int eq = line.indexOf(QLatin1Char('='));
      const QStringList rgb = line.mid(eq + 1).split(QLatin1Char(','));
      if (eq > 0 && rgb.size() == 3) {
        colors.insert(group + line.left(eq), QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()));
      }
    }
    return colors;
  }

  // Simplified KColorScheme::createApplicationPalette(): no HCY shading, Breeze's disabled contrast fade only
  std::optional<QPalette> kdePalette(const QString &path) {
    const QHash<QString, QColor> colors = readKdeColorScheme(path);
    const auto color = [&colors](const char *set, const char *key) {
      return colors.value(QStringLiteral("[Colors:%1]%2").arg(QLatin1String(set), QLatin1String(key)));
    };
    struct Role {
      QPalette::ColorRole role;
      const char *set;
      const char *key;
    };
    const Role roles[] = {
      {QPalette::Window, "Window", "BackgroundNormal"},
      {QPalette::WindowText, "Window", "ForegroundNormal"},
      {QPalette::Base, "View", "BackgroundNormal"},
      {QPalette::AlternateBase, "View", "BackgroundAlternate"},
      {QPalette::Text, "View", "ForegroundNormal"},
      {QPalette::PlaceholderText, "View", "ForegroundInactive"},
      {QPalette::Link, "View", "ForegroundLink"},
      {QPalette::LinkVisited, "View", "ForegroundVisited"},
      {QPalette::Button, "Button", "BackgroundNormal"},
      {QPalette::ButtonText, "Button", "ForegroundNormal"},
      {QPalette::Highlight, "Selection", "BackgroundNormal"},
      {QPalette::HighlightedText, "Selection", "ForegroundNormal"},
      {QPalette::ToolTipBase, "Tooltip", "BackgroundNormal"},
      {QPalette::ToolTipText, "Tooltip", "ForegroundNormal"},
    };

    QPalette palette;
    for (const auto &r : roles) {
      const QColor c = color(r.set, r.key);
      const QColor background = color(r.set, "BackgroundNormal");
      if (!c.isValid() || !background.isValid()) {
        return std::nullopt;
      }
      palette.setColor(r.role, c);
      if (QLatin1String(r.key).startsWith(QLatin1String("Foreground"))) {
        palette.setColor(QPalette::Disabled, r.role, mix(c, background, 0.65));
      }
    }

    const QColor window = palette.color(QPalette::Window);
    const QColor mid = window.darker(130);
    palette.setColor(QPalette::Light, window.lighter(150));
    palette.setColor(QPalette::Midlight, mid.lighter(110));
    palette.setColor(QPalette::Mid, mid);
    palette.setColor(QPalette::Dark, window.darker(150));
    palette.setColor(QPalette::Shadow, window.darker(150).darker(135));
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    palette.setColor(QPalette::Accent, palette.color(QPalette::Highlight));
#endif
    return palette;
  }

  bool isDark(const QPalette &palette) {
    return palette.color(QPalette::Window).lightness() < 128;
  }
#endif

  QString styleName(const QStyle *style) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
    return style->name();
#else
    return style->objectName();
#endif
  }
}

ColorSchemeOverride::Mode ColorSchemeOverride::modeFromString(const QString &mode) {
  if (mode == QLatin1String("light")) {
    return Mode::Light;
  }
  if (mode == QLatin1String("dark")) {
    return Mode::Dark;
  }
  return Mode::Auto;
}

void ColorSchemeOverride::apply(Mode mode) {
  restore();
  if (mode == Mode::Auto) {
    return;
  }
  const bool dark = mode == Mode::Dark;

  if (native_style.isEmpty()) {
    native_style = styleName(QApplication::style());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    native_palette = QApplication::palette();
#endif
  }

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  QGuiApplication::styleHints()->setColorScheme(dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
  scheme_requested = true;
#endif

#ifndef Q_OS_MACOS
  // QWindowsVistaStyle repaints any dark palette light
  if (dark && native_style.compare(QLatin1String("windowsvista"), Qt::CaseInsensitive) == 0) {
    QApplication::setStyle(QStringLiteral("Fusion"));
    style_swapped = true;
  }
  // The platform theme ignored the request: Plasma integration, xdg portal theme, Qt < 6.8
  if (isDark(QApplication::palette()) != dark) {
    const QString breeze = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                  dark ? QStringLiteral("color-schemes/BreezeDark.colors")
                                                       : QStringLiteral("color-schemes/BreezeLight.colors"));
    std::optional<QPalette> breeze_palette = kdePalette(breeze);
    if (breeze_palette) {
      const QColor accent = QApplication::palette().color(QPalette::Highlight);
      breeze_palette->setColor(QPalette::Active, QPalette::Highlight, accent);
      breeze_palette->setColor(QPalette::Inactive, QPalette::Highlight, accent);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
      breeze_palette->setColor(QPalette::Active, QPalette::Accent, accent);
      breeze_palette->setColor(QPalette::Inactive, QPalette::Accent, accent);
#endif
      // plasma-integration: per-app title bar colors, and no palette reset when the system scheme changes
      qApp->setProperty(kKdeColorSchemeProperty, breeze);
      QApplication::setPalette(*breeze_palette);
    } else {
      QApplication::setPalette(fusionPalette(dark));
    }
    palette_overridden = true;
  }
#endif
}

void ColorSchemeOverride::restore() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  if (scheme_requested) {
    QGuiApplication::styleHints()->unsetColorScheme();
    scheme_requested = false;
  }
#endif
  if (style_swapped) {
    QApplication::setStyle(native_style);
    style_swapped = false;
  }
  if (palette_overridden) {
    qApp->setProperty(kKdeColorSchemeProperty, QVariant());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QApplication::setPalette(QPalette());
#else
    QApplication::setPalette(native_palette);
#endif
    palette_overridden = false;
  }
}
