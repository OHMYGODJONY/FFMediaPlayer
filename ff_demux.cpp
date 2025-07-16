#include "ff_demux.h"
#include <iostream>
#include <string>
using namespace std;

extern "C" {
#include "libavformat/avformat.h"
}

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

static inline double r2d(AVRational r) 
{
    return r.den == 0 ? 0 : static_cast<double>(r.num) / static_cast<double>(r.den);
}

bool FF_Demux::Open(const char* url) 
{
    Close();
    //is_rtmp_ = string(url).find("rtmp://") == 0;
    is_rtmp_ = true;
    AVDictionary* opts = nullptr;
    if (is_rtmp_) 
    {
        // RTMP 直播流参数
        av_dict_set(&opts, "rtmp_live", "live", 0);
        cout << "Opening RTMP stream: " << url << endl;
    }
    // ============== RTSP 专用设置 ==============
    else if (string(url).find("rtsp://") == 0) 
    {
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
        av_dict_set(&opts, "timeout", "5000000", 0);  // 5秒超时
        av_dict_set(&opts, "stimeout", "5000000", 0);  // TCP socket超时
    }
    // ============== 通用网络设置 ==============
    else 
    {
        //av_dict_set(&opts, "timeout", "5000000", 0);
    }

    mux_.lock();
    int ret = avformat_open_input(&ifmt_ctx_, url, 0, &opts);
    av_dict_free(&opts);

    if (ret != 0) 
    {
        mux_.unlock();
        char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        cerr << "Open failed: " << buf << " (" << url << ")" << endl;
        return false;
    }
    cout << "open " << url << " success! " << endl;

    ret = avformat_find_stream_info(ifmt_ctx_, nullptr);
    if (ret < 0) {
        avformat_close_input(&ifmt_ctx_);
        mux_.unlock();
        cerr << "Find stream info failed." << endl;
        return false;
    }
    av_dump_format(ifmt_ctx_, 0, url, 0);

    video_index_ = av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_index_ < 0) 
    {
        mux_.unlock();
        cout << "No video stream found." << endl;
        avformat_close_input(&ifmt_ctx_);
        ifmt_ctx_ = nullptr;
        return false;  
    }
    cout << "Video stream index: " << video_index_ << endl;
    width = ifmt_ctx_->streams[video_index_]->codecpar->width;
    height = ifmt_ctx_->streams[video_index_]->codecpar->height;
    cout << "width=" << width << endl;
    cout << "height=" << height << endl;

    audio_index_ = av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_index_ < 0) 
    {
        mux_.unlock();
        cout << "No audio stream found." << endl;
        avformat_close_input(&ifmt_ctx_);
        ifmt_ctx_ = nullptr;
        return false;
    }
    cout << "Audio stream index: " << audio_index_ << endl;
    sample_rate = ifmt_ctx_->streams[audio_index_]->codecpar->sample_rate;
    channels = ifmt_ctx_->streams[audio_index_]->codecpar->channels;
    cout << "sample_rate = " << sample_rate << endl;
    cout << "channels = " << channels << endl;

    if (is_rtmp_ || ifmt_ctx_->duration == AV_NOPTS_VALUE) 
    {
        total_ms = 0;  // 直播流没有固定时长
        cout << "Live stream detected" << endl;
    }
    else 
    {
        total_ms = ifmt_ctx_->duration * 1000 / AV_TIME_BASE;
    }

    cout << "Opened: " << url << " | Duration: "
        << (total_ms > 0 ? to_string(total_ms) + "ms" : "LIVE") << endl;

    mux_.unlock();
    return true;
}

void FF_Demux::Clear()
{
    mux_.lock();
    if (!ifmt_ctx_)
    {
        mux_.unlock();
        return;
    }
    avformat_flush(ifmt_ctx_);
    mux_.unlock();
}

void FF_Demux::Close()
{
    mux_.lock();
    if (!ifmt_ctx_)
    {
        mux_.unlock();
        return;
    }
    avformat_close_input(&ifmt_ctx_);
    total_ms = 0;
    width = 0;
    height = 0;
    sample_rate = 0;
    channels = 0;
    video_index_ = -1;
    audio_index_ = -1;
    mux_.unlock();
}

AVCodecParameters* FF_Demux::Video_Para() 
{
    mux_.lock();
    if (!ifmt_ctx_ || video_index_ < 0) 
    {
        mux_.unlock();
        return nullptr;
    }
    AVCodecParameters* para = avcodec_parameters_alloc();
    avcodec_parameters_copy(para, ifmt_ctx_->streams[video_index_]->codecpar);
    mux_.unlock();
    return para;
}

AVCodecParameters* FF_Demux::Audio_Para() 
{
    mux_.lock();
    if (!ifmt_ctx_ || audio_index_ < 0) 
    {
        mux_.unlock();
        return nullptr;
    }
    AVCodecParameters* para = avcodec_parameters_alloc();
    avcodec_parameters_copy(para, ifmt_ctx_->streams[audio_index_]->codecpar);
    mux_.unlock();
    return para;
}

bool FF_Demux::Seek(double pos) 
{
    if (is_rtmp_) 
    {
        cerr << "Seek not supported on RTMP live stream." << endl;
        return false; // RTMP直播流不支持seek
    }
    if (pos < 0 || pos > 1.0) return false;
    mux_.lock();
    if (!ifmt_ctx_) 
    {
        mux_.unlock();
        return false;
    }
    avformat_flush(ifmt_ctx_);
    int seek_index = video_index_ >= 0 ? video_index_ : audio_index_;
    int64_t seekPos = static_cast<int64_t>(ifmt_ctx_->streams[seek_index]->duration * pos);
    int ret = av_seek_frame(ifmt_ctx_, seek_index, seekPos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
    mux_.unlock();
    return ret >= 0;
}

bool FF_Demux::Is_Audio(AVPacket* pkt) 
{
    if (!pkt) return false;
    return pkt->stream_index == audio_index_;
}

AVPacket* FF_Demux::Read() 
{
    mux_.lock();
    if (!ifmt_ctx_) 
    {
        mux_.unlock();
        return nullptr;
    }
    AVPacket* pkt = av_packet_alloc();
    int ret = av_read_frame(ifmt_ctx_, pkt);
    if (ret != 0) 
    {
        av_packet_free(&pkt);
        mux_.unlock();
        return nullptr;
    }
    AVStream* stream = ifmt_ctx_->streams[pkt->stream_index];
    pkt->pts = static_cast<int64_t>(pkt->pts * r2d(stream->time_base) * 1000);
    pkt->dts = static_cast<int64_t>(pkt->dts * r2d(stream->time_base) * 1000);
    mux_.unlock();
    return pkt;
}

AVPacket* FF_Demux::Read_Video() 
{
    mux_.lock();
    if (!ifmt_ctx_) 
    {
        mux_.unlock();
        return nullptr;
    }
    mux_.unlock();

    AVPacket* pkt = nullptr;
    for (int i = 0; i < 20; i++) 
    {
        pkt = Read();
        if (!pkt) break;
        if (pkt->stream_index == video_index_) {
            break; 
        }
        av_packet_free(&pkt);
    }
    return pkt;
}

FF_Demux::FF_Demux()
{
    static bool isFirst = true;
    static std::mutex dmux;
    dmux.lock();
    if (isFirst)
    {
        avformat_network_init();
        isFirst = false;
    }
    dmux.unlock();
}

FF_Demux::~FF_Demux() 
{
    Close();
}