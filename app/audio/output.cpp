#include "output.h"

#include <QDebug>
#include <QRandomGenerator>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
static void write_sample_s16ne(char *ptr, double sample) {
  int16_t *buf = (int16_t *)ptr;
  double range = (double)INT16_MAX - (double)INT16_MIN;
  double val = sample * range / 2.0;
  *buf = val;
}

static void write_sample_s32ne(char *ptr, double sample) {
  int32_t *buf = (int32_t *)ptr;
  double range = (double)INT32_MAX - (double)INT32_MIN;
  double val = sample * range / 2.0;
  *buf = val;
}

static void write_sample_float32ne(char *ptr, double sample) {
  float *buf = (float *)ptr;
  *buf = sample;
}

static void write_sample_float64ne(char *ptr, double sample) {
  double *buf = (double *)ptr;
  *buf = sample;
}
#pragma clang diagnostic pop

static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
  Q_UNUSED(frame_count_min)
  Audio::OutputPrivate::Context *ctx = static_cast<Audio::OutputPrivate::Context *>(outstream->userdata);
  struct SoundIoChannelArea *areas = nullptr;
  int frames_left = frame_count_max;

  Q_ASSERT(ctx);
  Q_ASSERT(ctx->write_sample);

  int err = 0;

  while (frames_left > 0) {
    int frame_count = frames_left;

    err = soundio_outstream_begin_write(outstream, &areas, &frame_count);
    if (err) {
      qCritical() << soundio_strerror(err);
      exit(1);
    }

    if (frame_count == 0) {
      break;
    }

    for (int frame = 0; frame < frame_count; frame++) {
      for (int channel = 0; channel < outstream->layout.channel_count; channel++) {
        double sample = ctx->readSample(channel);
        ctx->write_sample(areas[channel].ptr, sample);

        areas[channel].ptr += areas[channel].step;
      }
    }

    if ((err = soundio_outstream_end_write(outstream))) {
      qCritical() << soundio_strerror(err);
      exit(1);
    }

    frames_left -= frame_count;
  }

  soundio_outstream_pause(outstream, ctx->pause());
}

static void underflow_callback(struct SoundIoOutStream *outstream) {
  Q_UNUSED(outstream)
  static int count = 0;
  qWarning() << "underflow" << count;
  count++;
}

namespace Audio {
  Output::Output(Decoder *d) : context(OutputPrivate::Context(d)) {
    soundio = soundio_create();
    if (soundio == nullptr) {
      qWarning() << "cannot create soundio";
    } else {
      if (!init()) {
        qWarning() << "cannot initialize soundio";
      }
    }
  }

  Output::~Output() {
    if (soundio != nullptr) {
      soundio_outstream_destroy(outstream);
      soundio_device_unref(device);
      soundio_disconnect(soundio);
      soundio_destroy(soundio);
    }
  }

  bool Output::init() {
    int err = soundio_connect(soundio);
    if (err) {
      qWarning() << "cannot connect soundio:" << soundio_strerror(err);
      return false;
    }

    if (!connect_backend(SoundIoBackendPulseAudio)) {
      if (!connect_backend(SoundIoBackendAlsa)) {
        qWarning() << "cannot connect to any soundio backend";
        return false;
      }
    }

    soundio_flush_events(soundio);

    int default_out_device_index = soundio_default_output_device_index(soundio);
    if (default_out_device_index < 0) {
      qWarning() << "no output device found";
      return false;
    }

    device = soundio_get_output_device(soundio, default_out_device_index);
    if (device == nullptr) {
      qWarning() << "cannot get soundio device";
      return false;
    }

    qDebug() << "output device:" << device->name;
    soundio->app_name = "mpz";

    outstream = soundio_outstream_create(device);
    if (outstream == nullptr) {
      qWarning() << "cannot get output stream";
      return false;
    }
    outstream->userdata = static_cast<void *>(&context);
    Q_ASSERT(outstream->userdata);

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->name = "mpz playback";
    outstream->format = SoundIoFormatFloat32NE;

    if (soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
        outstream->format = SoundIoFormatFloat32NE;
        context.write_sample = write_sample_float32ne;
    } else if (soundio_device_supports_format(device, SoundIoFormatFloat64NE)) {
        outstream->format = SoundIoFormatFloat64NE;
        context.write_sample = write_sample_float64ne;
    } else if (soundio_device_supports_format(device, SoundIoFormatS32NE)) {
        outstream->format = SoundIoFormatS32NE;
        context.write_sample = write_sample_s32ne;
    } else if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {
        outstream->format = SoundIoFormatS16NE;
        context.write_sample = write_sample_s16ne;
    } else {
      qDebug() << "no suitable device format available";
      return false;
    }

    err = soundio_outstream_open(outstream);
    if (err) {
      qWarning() << "unable to open device" << soundio_strerror(err);
      return false;
    }
    if (device->probe_error) {
      qWarning() << "cannot probe device" << soundio_strerror(err);
      return false;
    }

    if (outstream->layout_error) {
      qWarning() << "unable to set channel layout" << soundio_strerror(outstream->layout_error);
    }

    err = soundio_outstream_start(outstream);
    if (err) {
      qWarning() << "unable to start device" << soundio_strerror(err);
      return false;
    }

    soundio_flush_events(soundio);

    return true;
  }

  bool Output::connect_backend(SoundIoBackend backend) {
    int count = soundio_backend_count(soundio);
    for (int i = 0; i < count; i++) {
      if (soundio_get_backend(soundio, i) == backend) {
        soundio_connect_backend(soundio, backend);
        return true;
      }
    }
    return false;
  }

  namespace OutputPrivate {
    Context::Context(Decoder *d) : decoder(d), _pause(false) {
      Q_ASSERT(decoder);
      write_sample = nullptr;
    }

    bool Context::pause() const {
      return _pause;
    }

    void Context::setPause(bool v) {
      _pause = v;
    }

    double Context::readSample(int channel) {
      qDebug() << channel;
      return QRandomGenerator::global()->generateDouble();
    }
  }
}


