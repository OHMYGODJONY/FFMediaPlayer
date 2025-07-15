#pragma once
struct AVCodecParameters;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
#include <mutex>

extern void FreePacket(AVPacket** pkt);
extern void FreeFrame(AVFrame** frame);

class FF_Decode
{
public:
	virtual bool Open(AVCodecParameters* para);
	virtual bool Send(AVPacket* pkt);
	virtual AVFrame* Recv();
	virtual void Close();
	virtual void Clear();

	bool isAudio = false;
	long long pts = 0;

	FF_Decode();
	virtual ~FF_Decode();

protected:
	AVCodecContext* decode_ctx_ = nullptr;
	std::mutex mux_;
};


