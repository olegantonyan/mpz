#ifndef WINDOWSTASKBAR_H
#define WINDOWSTASKBAR_H

#include "playback/playbackcontroller.h"

#include <QObject>
#include <QAbstractNativeEventFilter>

class QWidget;

class WindowsTaskbar : public QObject, public QAbstractNativeEventFilter {
  Q_OBJECT
public:
  explicit WindowsTaskbar(Playback::Controller *player, QWidget *window, QObject *parent = nullptr);
  ~WindowsTaskbar() override;

  static void setAppUserModelId();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  using native_result_t = qintptr;
#else
  using native_result_t = long;
#endif
  bool nativeEventFilter(const QByteArray &eventType, void *message, native_result_t *result) override;

private:
  void buildIcons();
  void ensureTaskbarList();
  void addThumbButtons();
  void updateButtons();
  void updateProgress(int seconds);
  void updateOverlay();
  void onThumbClick(int id);

  Playback::Controller *player;

  struct Impl;
  Impl *d;
};

#endif // WINDOWSTASKBAR_H
