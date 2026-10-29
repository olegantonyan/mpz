#ifndef MPZAPPLICATION_H
#define MPZAPPLICATION_H

#include "colorschemeoverride.h"

#include <QApplication>
#include <QStringList>

class MpzApplication : public QApplication {
  Q_OBJECT
public:
  MpzApplication(int &argc, char **argv);

  QStringList drainPendingFiles();

public slots:
  void applyColorScheme(const QString &scheme);

signals:
  void filesOpened(const QStringList &files);
  void activated();
  void paletteChanged();

protected:
  bool event(QEvent *event) override;

private:
  QStringList pending_files;
  ColorSchemeOverride color_scheme_override;
};

#endif // MPZAPPLICATION_H
