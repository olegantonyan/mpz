#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif

#include "windowsdarktitlebar.h"

#include <QGuiApplication>
#include <QPalette>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>

namespace {
  // 20 since Windows 10 build 18985, 19 on 1809-1903. Spelled out because MinGW's
  // dwmapi.h doesn't always declare either.
  constexpr DWORD IMMERSIVE_DARK_MODE = 20;
  constexpr DWORD IMMERSIVE_DARK_MODE_LEGACY = 19;
}

namespace WindowsDarkTitleBar {
  void apply(QWidget *window) {
    if (window == nullptr) {
      return;
    }
    auto hwnd = reinterpret_cast<HWND>(window->winId());
    if (hwnd == nullptr) {
      return;
    }

    const BOOL dark = QGuiApplication::palette().color(QPalette::Active, QPalette::Window).lightness() < 128;
    if (FAILED(DwmSetWindowAttribute(hwnd, IMMERSIVE_DARK_MODE, &dark, sizeof(dark)))) {
      DwmSetWindowAttribute(hwnd, IMMERSIVE_DARK_MODE_LEGACY, &dark, sizeof(dark));
    }
    // Windows 10 keeps the old frame until something invalidates it
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  }
}
