#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif

#include "windowstaskbar.h"

#include "track.h"
#include "playback/controls.h"

#include <QApplication>
#include <QCoreApplication>
#include <QStyle>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QToolButton>

#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shobjidl.h>

#ifndef THBN_CLICKED
#define THBN_CLICKED 0x1800
#endif

namespace {
  enum { THB_ID_PREV = 1, THB_ID_PLAYPAUSE = 2, THB_ID_NEXT = 3 };

  // Hardcoded to avoid linking the GUID library (uuid.lib / -luuid).
  const GUID kCLSID_TaskbarList = { 0x56FDF344, 0xFD6D, 0x11d0,
    { 0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90 } };

  HICON iconFromPixmap(const QPixmap &pm) {
    if (pm.isNull()) {
      return nullptr;
    }
    const QImage img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    const int w = img.width();
    const int h = img.height();

    BITMAPV5HEADER bi{};
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = w;
    bi.bV5Height = -h;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    HDC hdc = GetDC(nullptr);
    void *bits = nullptr;
    HBITMAP color = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdc);
    if (!color || !bits) {
      if (color) {
        DeleteObject(color);
      }
      return nullptr;
    }

    // ARGB32 bytes are B,G,R,A on little-endian == the BGRA DIB layout.
    const int stride = w * 4;
    for (int y = 0; y < h; ++y) {
      memcpy(static_cast<uchar *>(bits) + y * stride, img.constScanLine(y), stride);
    }

    HBITMAP mask = CreateBitmap(w, h, 1, 1, nullptr);
    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmColor = color;
    ii.hbmMask = mask;
    HICON hicon = CreateIconIndirect(&ii);
    DeleteObject(color);
    DeleteObject(mask);
    return hicon;
  }

  void fill_buttons(THUMBBUTTON *b, bool playing, bool stopped, HICON prev, HICON playpause, HICON next) {
    auto set_tip = [](THUMBBUTTON &btn, const QString &s) {
      const std::wstring w = s.toStdWString();
      lstrcpynW(btn.szTip, w.c_str(), ARRAYSIZE(btn.szTip));
    };

    b[0].dwMask = static_cast<THUMBBUTTONMASK>(THB_ICON | THB_TOOLTIP | THB_FLAGS);
    b[0].iId = THB_ID_PREV;
    b[0].hIcon = prev;
    set_tip(b[0], QCoreApplication::translate("WindowsTaskbar", "Previous"));
    b[0].dwFlags = stopped ? THBF_DISABLED : THBF_ENABLED;

    b[1].dwMask = static_cast<THUMBBUTTONMASK>(THB_ICON | THB_TOOLTIP | THB_FLAGS);
    b[1].iId = THB_ID_PLAYPAUSE;
    b[1].hIcon = playpause;
    set_tip(b[1], playing ? QCoreApplication::translate("WindowsTaskbar", "Pause")
                          : QCoreApplication::translate("WindowsTaskbar", "Play"));
    b[1].dwFlags = THBF_ENABLED;

    b[2].dwMask = static_cast<THUMBBUTTONMASK>(THB_ICON | THB_TOOLTIP | THB_FLAGS);
    b[2].iId = THB_ID_NEXT;
    b[2].hIcon = next;
    set_tip(b[2], QCoreApplication::translate("WindowsTaskbar", "Next"));
    b[2].dwFlags = stopped ? THBF_DISABLED : THBF_ENABLED;
  }
}

struct WindowsTaskbar::Impl {
  HWND hwnd = nullptr;
  UINT wm_tbc = 0;
  ITaskbarList3 *taskbar = nullptr;
  bool buttons_added = false;

  HICON prev_icon = nullptr;
  HICON next_icon = nullptr;
  HICON play_icon = nullptr;
  HICON pause_icon = nullptr;
};

WindowsTaskbar::WindowsTaskbar(Playback::Controller *pl, QWidget *window, QObject *parent)
  : QObject(parent), player(pl), d(new Impl) {
  d->hwnd = reinterpret_cast<HWND>(window->winId());
  d->wm_tbc = RegisterWindowMessageW(L"TaskbarButtonCreated");
  ChangeWindowMessageFilterEx(d->hwnd, d->wm_tbc, MSGFLT_ALLOW, nullptr);

  buildIcons();

  qApp->installNativeEventFilter(this);

  connect(player, &Playback::Controller::started, this, [this](const Track &) {
    updateButtons();
    updateOverlay();
    updateProgress(player->position());
  });
  connect(player, &Playback::Controller::paused, this, [this](const Track &) {
    updateButtons();
    updateOverlay();
    updateProgress(player->position());
  });
  connect(player, &Playback::Controller::stopped, this, [this]() {
    updateButtons();
    updateOverlay();
    updateProgress(0);
  });
  connect(player, &Playback::Controller::progress, this, [this](const Track &, int seconds) {
    updateProgress(seconds);
  });
  connect(player, &Playback::Controller::trackChanged, this, [this](const Track &) {
    updateButtons();
    updateProgress(player->position());
  });
}

WindowsTaskbar::~WindowsTaskbar() {
  qApp->removeNativeEventFilter(this);
  if (d->taskbar) {
    d->taskbar->SetOverlayIcon(d->hwnd, nullptr, L"");
    d->taskbar->SetProgressState(d->hwnd, TBPF_NOPROGRESS);
    d->taskbar->Release();
    d->taskbar = nullptr;
  }
  if (d->prev_icon) {
    DestroyIcon(d->prev_icon);
  }
  if (d->next_icon) {
    DestroyIcon(d->next_icon);
  }
  if (d->play_icon) {
    DestroyIcon(d->play_icon);
  }
  if (d->pause_icon) {
    DestroyIcon(d->pause_icon);
  }
  delete d;
}

void WindowsTaskbar::setAppUserModelId() {
  SetCurrentProcessExplicitAppUserModelID(L"mpz-player.mpz");
}

void WindowsTaskbar::buildIcons() {
  int w = GetSystemMetrics(SM_CXSMICON);
  int h = GetSystemMetrics(SM_CYSMICON);
  const QSize sz(w > 0 ? w : 16, h > 0 ? h : 16);
  QStyle *style = QApplication::style();
  d->prev_icon = iconFromPixmap(style->standardIcon(QStyle::SP_MediaSkipBackward).pixmap(sz));
  d->next_icon = iconFromPixmap(style->standardIcon(QStyle::SP_MediaSkipForward).pixmap(sz));
  d->play_icon = iconFromPixmap(style->standardIcon(QStyle::SP_MediaPlay).pixmap(sz));
  d->pause_icon = iconFromPixmap(style->standardIcon(QStyle::SP_MediaPause).pixmap(sz));
}

void WindowsTaskbar::ensureTaskbarList() {
  if (d->taskbar) {
    return;
  }
  ITaskbarList3 *tb = nullptr;
  HRESULT hr = CoCreateInstance(kCLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&tb));
  if (FAILED(hr) || !tb) {
    return;
  }
  if (FAILED(tb->HrInit())) {
    tb->Release();
    return;
  }
  d->taskbar = tb;
}

void WindowsTaskbar::addThumbButtons() {
  if (!d->taskbar || d->buttons_added) {
    return;
  }
  const bool playing = player->state() == Playback::Controller::Playing;
  const bool stopped = player->state() == Playback::Controller::Stopped;
  THUMBBUTTON b[3] = {};
  fill_buttons(b, playing, stopped, d->prev_icon, playing ? d->pause_icon : d->play_icon, d->next_icon);
  if (SUCCEEDED(d->taskbar->ThumbBarAddButtons(d->hwnd, 3, b))) {
    d->buttons_added = true;
  }
}

void WindowsTaskbar::updateButtons() {
  if (!d->taskbar || !d->buttons_added) {
    return;
  }
  const bool playing = player->state() == Playback::Controller::Playing;
  const bool stopped = player->state() == Playback::Controller::Stopped;
  THUMBBUTTON b[3] = {};
  fill_buttons(b, playing, stopped, d->prev_icon, playing ? d->pause_icon : d->play_icon, d->next_icon);
  d->taskbar->ThumbBarUpdateButtons(d->hwnd, 3, b);
}

void WindowsTaskbar::updateProgress(int seconds) {
  if (!d->taskbar) {
    return;
  }
  const Track &t = player->currentTrack();
  const Playback::Controller::State st = player->state();
  if (st == Playback::Controller::Stopped || t.isStream() || t.duration() == 0) {
    d->taskbar->SetProgressState(d->hwnd, TBPF_NOPROGRESS);
    return;
  }
  d->taskbar->SetProgressState(d->hwnd, st == Playback::Controller::Paused ? TBPF_PAUSED : TBPF_NORMAL);
  ULONGLONG total = t.duration() / 1000;
  ULONGLONG cur = seconds < 0 ? 0 : static_cast<ULONGLONG>(seconds);
  if (total > 0 && cur > total) {
    cur = total;
  }
  d->taskbar->SetProgressValue(d->hwnd, cur, total > 0 ? total : 1);
}

void WindowsTaskbar::updateOverlay() {
  if (!d->taskbar) {
    return;
  }
  HICON icon = nullptr;
  const wchar_t *desc = L"";
  switch (player->state()) {
    case Playback::Controller::Playing: icon = d->play_icon; desc = L"Playing"; break;
    case Playback::Controller::Paused: icon = d->pause_icon; desc = L"Paused"; break;
    case Playback::Controller::Stopped: icon = nullptr; desc = L""; break;
  }
  d->taskbar->SetOverlayIcon(d->hwnd, icon, desc);
}

void WindowsTaskbar::onThumbClick(int id) {
  const Playback::Controls c = player->controls();
  switch (id) {
    case THB_ID_PREV:
      c.prev->click();
      break;
    case THB_ID_NEXT:
      c.next->click();
      break;
    case THB_ID_PLAYPAUSE:
      if (player->state() == Playback::Controller::Playing) {
        c.pause->click();
      } else {
        c.play->click();
      }
      break;
    default:
      break;
  }
}

bool WindowsTaskbar::nativeEventFilter(const QByteArray &, void *message, native_result_t *) {
  MSG *msg = static_cast<MSG *>(message);
  if (!msg || msg->hwnd != d->hwnd) {
    return false;
  }
  if (d->wm_tbc != 0 && msg->message == d->wm_tbc) {
    d->buttons_added = false;
    ensureTaskbarList();
    addThumbButtons();
    updateButtons();
    updateOverlay();
    updateProgress(player->position());
    return false;
  }
  if (msg->message == WM_COMMAND && HIWORD(msg->wParam) == THBN_CLICKED) {
    onThumbClick(LOWORD(msg->wParam));
    return true;
  }
  return false;
}
