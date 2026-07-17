#include "windowsmediacontrols.h"

#include "track.h"
#include "coverart/online/downloader.h"
#include "playback/controls.h"

#include <QDir>
#include <QMetaObject>
#include <QToolButton>

#include <chrono>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <systemmediatransportcontrolsinterop.h>

using winrt::Windows::Foundation::AsyncStatus;
using winrt::Windows::Foundation::IAsyncOperation;
using winrt::Windows::Foundation::TimeSpan;
using winrt::Windows::Media::MediaPlaybackStatus;
using winrt::Windows::Media::MediaPlaybackType;
using winrt::Windows::Media::PlaybackPositionChangeRequestedEventArgs;
using winrt::Windows::Media::SystemMediaTransportControls;
using winrt::Windows::Media::SystemMediaTransportControlsButton;
using winrt::Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs;
using winrt::Windows::Media::SystemMediaTransportControlsTimelineProperties;
using winrt::Windows::Storage::StorageFile;
using winrt::Windows::Storage::Streams::RandomAccessStreamReference;

namespace {
  winrt::hstring to_hstring(const QString &s) {
    return winrt::hstring{ s.toStdWString() };
  }
}

struct WindowsMediaControls::Impl {
  SystemMediaTransportControls smtc{ nullptr };
};

WindowsMediaControls::WindowsMediaControls(Playback::Controller *pl, QWidget *window, QObject *parent)
  : QObject(parent), player(pl), d(new Impl) {
  try {
    // Qt already inits the GUI thread's COM apartment; tolerate the mismatch.
    winrt::init_apartment(winrt::apartment_type::single_threaded);
  } catch (winrt::hresult_error const &) {
  }

  HWND hwnd = reinterpret_cast<HWND>(window->winId());
  auto interop = winrt::get_activation_factory<SystemMediaTransportControls, ISystemMediaTransportControlsInterop>();
  winrt::check_hresult(interop->GetForWindow(hwnd, winrt::guid_of<SystemMediaTransportControls>(), winrt::put_abi(d->smtc)));

  d->smtc.IsEnabled(true);
  d->smtc.IsPlayEnabled(true);
  d->smtc.IsPauseEnabled(true);
  d->smtc.IsStopEnabled(true);
  d->smtc.IsNextEnabled(true);
  d->smtc.IsPreviousEnabled(true);
  updateState(player->state());

  // SMTC callbacks arrive on a WinRT pool thread; marshal to the GUI thread before touching widgets.
  d->smtc.ButtonPressed([this](SystemMediaTransportControls const &, SystemMediaTransportControlsButtonPressedEventArgs const &args) {
    const SystemMediaTransportControlsButton button = args.Button();
    QMetaObject::invokeMethod(this, [this, button]() {
      const Playback::Controls c = player->controls();
      switch (button) {
        // Windows picks Play vs Pause from the PlaybackStatus we reported and gets it wrong in the background.
        case SystemMediaTransportControlsButton::Play:
        case SystemMediaTransportControlsButton::Pause:
          if (player->state() == Playback::Controller::Playing) {
            c.pause->click();
          } else {
            c.play->click();
          }
          break;
        case SystemMediaTransportControlsButton::Stop:     c.stop->click(); break;
        case SystemMediaTransportControlsButton::Next:     c.next->click(); break;
        case SystemMediaTransportControlsButton::Previous: c.prev->click(); break;
        default: break;
      }
    }, Qt::QueuedConnection);
  });

  d->smtc.PlaybackPositionChangeRequested([this](SystemMediaTransportControls const &, PlaybackPositionChangeRequestedEventArgs const &args) {
    const int seconds = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(args.RequestedPlaybackPosition()).count());
    QMetaObject::invokeMethod(this, [this, seconds]() {
      player->seek(seconds);
    }, Qt::QueuedConnection);
  });

  connect(player, &Playback::Controller::started, this, [this](const Track &) {
    updateState(Playback::Controller::Playing);
    updateMetadata();
    updateTimeline();
  });
  connect(player, &Playback::Controller::paused, this, [this](const Track &) {
    updateState(Playback::Controller::Paused);
    updateMetadata();
    updateTimeline();
  });
  connect(player, &Playback::Controller::stopped, this, [this]() {
    auto updater = d->smtc.DisplayUpdater();
    updater.ClearAll();
    updater.Update();
    updateState(Playback::Controller::Stopped);
  });
  connect(player, &Playback::Controller::trackChanged, this, [this](const Track &) {
    updateMetadata();
    updateTimeline();
  });
  connect(player, &Playback::Controller::seeked, this, [this](int) {
    updateTimeline();
  });
  // updateMetadata() nulls the thumbnail before reading artCover(), so without
  // this the flyout stays blank for the whole track a cover is downloaded on.
  connect(&CoverArt::Online::Downloader::instance(), &CoverArt::Online::Downloader::coverAvailable,
          this, [this](const QString &artist, const QString &album, const QString &) {
    const Track current = player->currentTrack();
    if (current.isValid() && current.artist() == artist && current.album() == album) {
      updateMetadata();
    }
  });
}

WindowsMediaControls::~WindowsMediaControls() {
  if (d->smtc) {
    try {
      auto updater = d->smtc.DisplayUpdater();
      updater.ClearAll();
      updater.Update();
      d->smtc.PlaybackStatus(MediaPlaybackStatus::Stopped);
      d->smtc.IsEnabled(false);
    } catch (winrt::hresult_error const &) {
    }
  }
  delete d;
}

void WindowsMediaControls::updateMetadata() {
  const Track &track = player->currentTrack();
  auto updater = d->smtc.DisplayUpdater();
  updater.Type(MediaPlaybackType::Music);

  auto music = updater.MusicProperties();
  music.Title(to_hstring(track.title()));
  music.Artist(to_hstring(track.artist()));
  music.AlbumTitle(to_hstring(track.album()));
  if (track.track_number() > 0) {
    music.TrackNumber(track.track_number());
  }

  updater.Thumbnail(nullptr);
  updater.Update();

  const QString art = track.artCover();
  if (art.isEmpty()) {
    return;
  }

  // CreateFromUri(file://) renders a black box in SMTC; resolve a StorageFile and
  // use CreateFromFile. The async completes on a pool thread — fine, DisplayUpdater is agile.
  const winrt::hstring path = to_hstring(QDir::toNativeSeparators(art));
  try {
    auto op = StorageFile::GetFileFromPathAsync(path);
    op.Completed([updater](IAsyncOperation<StorageFile> const &operation, AsyncStatus status) {
      if (status != AsyncStatus::Completed) {
        return;
      }
      try {
        updater.Thumbnail(RandomAccessStreamReference::CreateFromFile(operation.GetResults()));
        updater.Update();
      } catch (winrt::hresult_error const &) {
      }
    });
  } catch (winrt::hresult_error const &) {
  }
}

void WindowsMediaControls::updateState(Playback::Controller::State state) {
  MediaPlaybackStatus status = MediaPlaybackStatus::Stopped;
  switch (state) {
    case Playback::Controller::Playing: status = MediaPlaybackStatus::Playing; break;
    case Playback::Controller::Paused:  status = MediaPlaybackStatus::Paused; break;
    case Playback::Controller::Stopped: status = MediaPlaybackStatus::Stopped; break;
  }
  d->smtc.PlaybackStatus(status);
}

void WindowsMediaControls::updateTimeline() {
  using namespace std::chrono;
  const Track &track = player->currentTrack();

  d->smtc.PlaybackRate(player->state() == Playback::Controller::Playing ? 1.0 : 0.0);

  // All-zero props hide the scrubber — wanted for streams (no duration).
  SystemMediaTransportControlsTimelineProperties props;
  if (!track.isStream() && track.duration() > 0) {
    props.StartTime(TimeSpan{ 0 });
    props.MinSeekTime(TimeSpan{ 0 });
    props.EndTime(milliseconds{ track.duration() });
    props.MaxSeekTime(milliseconds{ track.duration() });
    props.Position(seconds{ player->position() });
  }
  d->smtc.UpdateTimelineProperties(props);
}
