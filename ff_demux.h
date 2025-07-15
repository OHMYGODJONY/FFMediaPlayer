#pragma once
#include <mutex>
struct AVPacket;
struct AVCodecParameters;
struct AVFormatContext;
class FF_Demux
{
public:
	virtual bool Open(const char* url);
	virtual AVPacket* Read();
	virtual AVPacket* Read_Video();
	virtual bool Is_Audio(AVPacket* pkt);
	virtual AVCodecParameters* Video_Para();
	virtual AVCodecParameters* Audio_Para();
	virtual bool Seek(double pos);

	virtual void Clear();
	virtual void Close();

	FF_Demux();
	virtual ~FF_Demux();
	int total_ms = 0;
	int width = 0;
	int height = 0;
	int sample_rate = 0;
	int channels = 0;

protected:
	std::mutex mux_;
	AVFormatContext* ifmt_ctx_ = NULL;
	int video_index_ = -1;
	int audio_index_ = -1;
};

