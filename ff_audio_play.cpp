#include "ff_audio_play.h"
#include <QAudioDevice>
#include <QMediaDevices>
#include <QAudioSink>
#include <mutex>

class CXAudioPlay : public FF_Audio_Play
{
public:
    QAudioSink* sink = nullptr;
    QIODevice* io = nullptr;
    std::mutex mux;
    QAudioFormat format;

    virtual long long GetNoPlayMs() override
    {
        std::lock_guard<std::mutex> lock(mux);
        if (!sink) return 0;

        // 计算已缓冲但未播放的数据量
        qint64 bytesBuffered = sink->bufferSize() - sink->bytesFree();

        // 计算对应的毫秒数
        double bytesPerMillisecond = (sampleRate * channels * (sampleSize / 8)) / 1000.0;
        if (bytesPerMillisecond <= 0) return 0;

        return static_cast<long long>(bytesBuffered / bytesPerMillisecond);
    }

    void Clear() override
    {
        std::lock_guard<std::mutex> lock(mux);
        if (io) io->reset();
    }

    virtual void Close() override
    {
        std::lock_guard<std::mutex> lock(mux);
        if (io) {
            io->close();
            io = nullptr;
        }
        if (sink) {
            sink->stop();
            delete sink;
            sink = nullptr;
        }
    }

    virtual bool Open() override
    {
        Close();

        format.setSampleRate(sampleRate);
        format.setChannelCount(channels);

        // Qt6采样格式设置
        QAudioFormat::SampleFormat sampleFormat = QAudioFormat::Unknown;
        if (sampleSize == 8) {
            sampleFormat = QAudioFormat::UInt8;
        }
        else if (sampleSize == 16) {
            sampleFormat = QAudioFormat::Int16;
        }
        else if (sampleSize == 32) {
            sampleFormat = QAudioFormat::Float;
        }
        format.setSampleFormat(sampleFormat);

        // 获取默认输出设备
        QAudioDevice device = QMediaDevices::defaultAudioOutput();
        if (!device.isFormatSupported(format)) {
            format = device.preferredFormat();
        }

        std::lock_guard<std::mutex> lock(mux);
        sink = new QAudioSink(device, format);
        io = sink->start();

        return io != nullptr;
    }

    void SetPause(bool isPause) override
    {
        std::lock_guard<std::mutex> lock(mux);
        if (!sink) return;

        isPause ? sink->suspend() : sink->resume();
    }

    virtual bool Write(const unsigned char* data, int datasize) override
    {
        if (!data || datasize <= 0) return false;

        mux.lock();
        if (!sink || !io)
        {
            mux.unlock();
            return false;
        }

        int size = io->write(reinterpret_cast<const char*>(data), datasize);
        mux.unlock();
        if (datasize != size)
        {
            return false;
        }
        return true;
    }

    virtual int GetFree() override
    {
        std::lock_guard<std::mutex> lock(mux);
        return sink ? sink->bytesFree() : 0;
    }
};

FF_Audio_Play* FF_Audio_Play::Get()
{
    static CXAudioPlay play;
    return &play;
}

FF_Audio_Play::FF_Audio_Play() = default;
FF_Audio_Play::~FF_Audio_Play() = default;