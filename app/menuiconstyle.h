#ifndef MENUICONSTYLE_H
#define MENUICONSTYLE_H

#include <QProxyStyle>
#include <QMenu>

class MenuIconStyle : public QProxyStyle {
public:
  int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override {
    if (metric == PM_SmallIconSize) {
      return QProxyStyle::pixelMetric(metric, option, widget) * 7 / 8;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
  }
};

inline void applyMenuIconStyle(QMenu *menu) {
  static MenuIconStyle style;
  menu->setStyle(&style);
}

#endif // MENUICONSTYLE_H
