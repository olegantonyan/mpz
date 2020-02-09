#include "decoder.h"

#include <QDebug>
#include <QDateTime>
#include <QFile>


static void decode(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame) {
    int i, ch;
    int ret, data_size;

    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
      qWarning() << "avcodec_send_packet error";
    }

    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
          qWarning() << "decoding error";
        }
        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if (data_size < 0) {
          qWarning() << "diata size error";
        }

        qDebug() << "samples" << frame->nb_samples;
        qDebug() << "channels" << dec_ctx->channels;

        /*for (i = 0; i < frame->nb_samples; i++) {
          for (ch = 0; ch < dec_ctx->channels; ch++) {

            fwrite(frame->data[ch] + data_size*i, 1, data_size, outfile);
          }
        }*/
    }
}

namespace Audio {
  Decoder::Decoder() : sample_pointer(0) {



    AVCodec *codec = nullptr;
    AVCodecContext *c= nullptr;
    AVCodecParserContext *parser = nullptr;
    AVPacket *pkt = av_packet_alloc();
    AVFrame *decoded_frame = nullptr;

    codec = avcodec_find_decoder(AV_CODEC_ID_MP2);
    if (!codec) {
      qWarning() << "cannot find codec";
    }
    parser = av_parser_init(codec->id);
    if (!parser) {
      qWarning() << "cannot find parser";
    }
    c = avcodec_alloc_context3(codec);
    if (!c) {
      qWarning() << "cannot allocate audio codec context";
    }

    if (avcodec_open2(c, codec, nullptr) < 0) {
      qWarning() << "cannot open codec";
    }

    auto filename = "/home/oleg/Desktop/file.mp3";
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
      qWarning() << "cannot open file" << f.errorString();
    }
    auto a = f.readAll();
    f.close();
    unsigned char *data = (unsigned char *)a.data();
    int data_size = a.size();
    qDebug() << "data size" << data_size;

    decoded_frame = av_frame_alloc();

    int ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    if (ret < 0) {
      qWarning() << "parse error";
    }
    //data += ret;
    qDebug() << "pkt size" << pkt->size;
    if (pkt->size) {
      decode(c, pkt, decoded_frame);
    }

    //av_register_all();

    /*AVCodec *codec = av_codec_next(nullptr);
    qDebug() << "enumerating codecs";
    while(codec != nullptr) {
      qDebug() << codec->long_name;
      codec = av_codec_next(codec);
    }

    AVOutputFormat *oformat = av_oformat_next(nullptr);
    qDebug() << "enumerating formats";
    while(oformat != nullptr) {
      qDebug() << oformat->long_name;
      oformat = av_oformat_next(oformat);
    }*/





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
