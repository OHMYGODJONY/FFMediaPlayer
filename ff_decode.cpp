#include "ff_decode.h"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <iostream>
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
        avcodec_parameters_free(&para);
        cout << "Decoder not found for codec ID: " << para->codec_id << endl;
        return false;
    }
    cout << "Decoder founded!!!!" << endl;

    mux_.lock();
    decode_ctx_ = avcodec_alloc_context3(codec);
    if (!decode_ctx_) 
    {
        avcodec_parameters_free(&para);
        mux_.unlock();
        cerr << "Failed to allocate codec context" << endl;
        return false;
    }

    if (avcodec_parameters_to_context(decode_ctx_, para) < 0) 
    {
        avcodec_free_context(&decode_ctx_);
        avcodec_parameters_free(&para);
        mux_.unlock();
        cerr << "Failed to copy parameters to context" << endl;
        return false;
    }
    avcodec_parameters_free(&para);

    decode_ctx_->thread_count = min(8, av_cpu_count());
    decode_ctx_->thread_type = FF_THREAD_FRAME;

    int re = avcodec_open2(decode_ctx_, codec, nullptr);
    if (re != 0) 
    {
        avcodec_free_context(&decode_ctx_);
        mux_.unlock();
        char buf[1024] = { 0 };
        av_strerror(re, buf, sizeof(buf) - 1);
        cout << "avcodec_open2  failed! :" << buf << endl;
        return false;
    }
    mux_.unlock();
    cout << "Opened decoder for " << codec->name << endl;
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
    return ret >= 0;
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
    int ret = avcodec_receive_frame(decode_ctx_, frame);
    mux_.unlock();
    if (ret < 0) 
    {
        av_frame_free(&frame);
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
}