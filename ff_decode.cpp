#include "ff_decode.h"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <iostream>
#include <memory>
using namespace std;

void FreePacket(AVPacket** pkt) 
{
    if (!pkt || !(*pkt)) return;
    av_packet_free(pkt);
}

void FreeFrame(AVFrame** frame) 
{
    if (!frame || !(*frame)) return;
    av_frame_free(frame);
}

bool FF_Decode::Open(AVCodecParameters* para) 
{
    if (!para) return false;

    Close();

    AVCodec* codec = avcodec_find_decoder(para->codec_id);
    if (!codec) 
    {
        cout << "Decoder not found for codec ID: " << para->codec_id << endl;
        avcodec_parameters_free(&para);
        return false;
    }
    cout << "Decoder founded!!!!" << endl;

    mux_.lock();
    decode_ctx_ = avcodec_alloc_context3(codec);
    if (!decode_ctx_) 
    {
        mux_.unlock();
        avcodec_parameters_free(&para);
        cerr << "Failed to allocate codec context" << endl;
        return false;
    }

    if (avcodec_parameters_to_context(decode_ctx_, para) < 0)
    {
        avcodec_free_context(&decode_ctx_);
        mux_.unlock();
        avcodec_parameters_free(&para);
        cerr << "Failed to copy parameters to context" << endl;
        return false;
    }

    if (para->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        // 视频解码优化
        decode_ctx_->thread_count = min(16, av_cpu_count() * 2);
        decode_ctx_->thread_type = FF_THREAD_FRAME;
        decode_ctx_->flags |= AV_CODEC_FLAG_LOW_DELAY;
    }
    else 
    {
        // 音频解码优化
        decode_ctx_->thread_count = min(8, av_cpu_count());
    }

    int re = avcodec_open2(decode_ctx_, codec, nullptr);
    if (re != 0) 
    {
        avcodec_free_context(&decode_ctx_);
        mux_.unlock();
        avcodec_parameters_free(&para);
        char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(re, buf, sizeof(buf) - 1);
        cerr << "avcodec_open2 failed: " << buf << endl;
        return false;
    }

    mux_.unlock();
    cout << "Opened " << (para->codec_type == AVMEDIA_TYPE_VIDEO ? "video" : "audio")
        << " decoder: " << codec->name << " (threads: " << decode_ctx_->thread_count << ")" << endl;
    avcodec_parameters_free(&para);

    return true;
}

bool FF_Decode::Send(AVPacket* pkt) 
{
    if (!pkt || pkt->size <= 0 || !pkt->data) return false;

    mux_.lock();
    if (!decode_ctx_) 
    {
        mux_.unlock();
        return false;
    }
    int ret = avcodec_send_packet(decode_ctx_, pkt);
    mux_.unlock();
    av_packet_free(&pkt);
    if (ret == AVERROR(EAGAIN)) 
    {
        // 解码器缓冲区已满，需要先接收帧
        return false;
    }
    else if (ret == AVERROR_EOF) 
    {
        // 解码器已刷新，需要重新打开
        cerr << "Decoder received EOF" << endl;
        return false;
    }
    else if (ret < 0) 
    {
        char buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, buf, sizeof(buf));
        cerr << "avcodec_send_packet error: " << buf << endl;
        return false;
    }
    return true;
}

AVFrame* FF_Decode::Recv() 
{
    mux_.lock();
    if (!decode_ctx_) 
    {
        mux_.unlock();
        return nullptr;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) 
    {
        cerr << "Failed to allocate frame" << endl;
        return nullptr;
    }

    int ret = avcodec_receive_frame(decode_ctx_, frame);
    mux_.unlock();
    if (ret < 0) 
    {
        av_frame_free(&frame);
        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) 
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            cerr << "avcodec_receive_frame error: " << errbuf << endl;
        }
        return nullptr;
    }
    pts = frame->pts;
    return frame;
}

void FF_Decode::Close() 
{
    mux_.lock();
    if (decode_ctx_) 
    {
        avcodec_close(decode_ctx_);
        avcodec_free_context(&decode_ctx_);
    }
    pts = 0;
    mux_.unlock();
}

void FF_Decode::Clear() 
{
    mux_.lock();
    if (decode_ctx_) 
    {
        avcodec_flush_buffers(decode_ctx_);
    }
    mux_.unlock();
}

FF_Decode::FF_Decode() 
{
}

FF_Decode::~FF_Decode() 
{
    Close();
}