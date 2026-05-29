/*
 * miniaudio implementation translation unit.
 *
 * The single-header library is vendored at 3rdparty/miniaudio/miniaudio.h and
 * pulled in here exactly once (MINIAUDIO_IMPLEMENTATION). Every other source
 * file includes miniaudio.h without the implementation macro.
 *
 * We use only the low-level device + context API (playback device control and
 * device enumeration) and the lock-free PCM ring buffer (ma_pcm_rb). All
 * decode/resample work is done by FFmpeg in app/playback/native/engine.cpp, so
 * miniaudio's decoders/encoders/waveform-generators and the high-level
 * engine/resource-manager are compiled out to keep the TU small.
 */
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_ENGINE

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
