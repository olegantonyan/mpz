#include "decoder.h"

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QThread>

#define DR_FLAC_IMPLEMENTATION
#include "extras/dr_flac.h"
#define DR_MP3_IMPLEMENTATION
#include "extras/dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#include "extras/dr_wav.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == nullptr) {
      return;
    }

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount);

    (void)pInput;
}

static size_t read_func(ma_decoder *decoder, void *buffer_out, size_t bytes_to_read) {
  QFile *f = (QFile *)decoder->pUserData;
  auto data = f->read(bytes_to_read);
  memcpy(buffer_out, data.data(), data.size());
  return data.size();
}

static ma_bool32 seek_func(ma_decoder *decoder, int byte_offset, ma_seek_origin origin) {
  return 0;
}

void log_callback(ma_context* pContext, ma_device* pDevice, ma_uint32 logLevel, const char* message)
{
    (void)pContext;
    (void)pDevice;
    printf("miniaudio: [%s] %s\n", ma_log_level_to_string(logLevel), message);
}

namespace Audio {
  Decoder::Decoder() : sample_pointer(0) {

    ma_result result;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_device device;

    QFile f("/home/oleg/Desktop/file.flac");
    if (!f.open(QIODevice::ReadOnly)) {
      qWarning() << "open file error" << f.errorString();
    }

    result = ma_decoder_init(read_func, seek_func, &f, nullptr, &decoder);
    if (result != MA_SUCCESS) {
      qWarning() << "init error";
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    qDebug() << "output format:" << decoder.outputFormat;
    qDebug() << "chanels:" << decoder.outputChannels;
    qDebug() << "sample rate:" << decoder.outputSampleRate;

    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &decoder;

    if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
        qWarning() << "device open error";
        ma_decoder_uninit(&decoder);
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        qWarning() << "playback start error";
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
    }

    QThread::currentThread()->sleep(100000000);


    _format.setSampleRate(44100);
    _format.setChannelCount(2);
    _format.setCodec("audio/pcm");
    //_format.setSampleType()
    //_format.setByteOrder()
  }

  double Decoder::readSample(bool *eof, int channel, int channels_count) {
    Q_ASSERT(eof);
    /*if (sample_pointer >= audioFile.getNumSamplesPerChannel()) {
      *eof = true;
      return 0.0;
    }

    double sample = audioFile.samples[channel][sample_pointer];
    if (channel + 1 >= channels_count) {
      sample_pointer++;
    }
    *eof = false;
    return sample;*/
    return 0.0;
  }

  const char *Decoder::streamName() const {
    return "audio playback";
  }

  const QAudioFormat &Decoder::format() const {
    return _format;
  }
}
