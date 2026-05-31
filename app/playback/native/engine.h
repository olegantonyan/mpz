#ifndef NATIVE_ENGINE_H
#define NATIVE_ENGINE_H

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Playback {
  namespace Native {

    struct DeviceInfo {
      std::string id;        // stable, opaque serialization of the platform device id
      std::string name;
      bool is_default = false;
    };

    // Qt-free audio engine: FFmpeg demux/decode -> libswresample -> a lock-free
    // PCM ring buffer -> miniaudio output device. The realtime audio callback
    // only touches the ring buffer and a few atomics; all decoding happens on a
    // dedicated worker thread. The Qt wrapper (Playback::Native::MediaPlayer)
    // marshals the callbacks below onto the GUI thread.
    //
    // The engine works in ABSOLUTE file time: it plays a single source from a
    // start offset to its natural end. CUE windowing (sub-track boundaries) is
    // layered on top by the wrapper, which keeps same-file CUE playback gapless
    // because the engine never stops at a sub-track boundary.
    //
    // PIMPL keeps the heavy miniaudio.h / FFmpeg headers out of this interface.
    class Engine {
    public:
      enum class State { Stopped, Playing, Paused };

      Engine();
      ~Engine();
      Engine(const Engine &) = delete;
      Engine &operator=(const Engine &) = delete;

      // Invoked from the decode thread; the wrapper queues them to the GUI thread.
      void setEndOfStreamCallback(std::function<void()> cb);   // natural end of source
      void setErrorCallback(std::function<void(const std::string &)> cb);
      // Stream (ICY) metadata: keys "icy-br", "icy-sr", "content-type", "stream"
      // (the raw StreamTitle='...'; blob). Fired on open and on mid-stream updates.
      void setMetadataCallback(std::function<void(const std::map<std::string, std::string> &)> cb);

      // Open a local path or http(s):// URL and seek to start_ms. Returns false
      // on failure (also reported via the error callback). Does not start output.
      bool open(const std::string &url, int64_t start_ms = 0);

      void play();
      void pause();
      void stop();
      void seek(int64_t abs_ms);      // absolute position in the source
      void setVolume(float gain01);   // 0.0 .. 1.0

      State state() const;
      int64_t positionMs() const;     // absolute position in the source
      int64_t durationMs() const;     // 0 if unknown (e.g. a live stream)
      bool isSeekable() const;

      // Output-device selection. pDeviceId is an opaque miniaudio ma_device_id*
      // (void* to keep the header platform-free); nullptr selects the system
      // default. Recreates the device, resuming from the current position.
      bool setDevice(const void *pDeviceId);

      // Enumerate playback devices and switch by serialized id. setDeviceById("")
      // selects the system default. Ids match DeviceInfo::id from playbackDevices().
      std::vector<DeviceInfo> playbackDevices();
      bool setDeviceById(const std::string &id);

    private:
      struct Impl;
      std::unique_ptr<Impl> d;
    };

  } // namespace Native
} // namespace Playback

#endif // NATIVE_ENGINE_H
