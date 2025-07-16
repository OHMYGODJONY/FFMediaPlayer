#pragma once
struct AVCodecParameters;
struct AVFrame;
struct SwrContext;
#include <mutex>
class FF_Resample
{
public:
	virtual bool Open(AVCodecParameters* para, bool isClearPara = false);
	virtual void Close();
	virtual int Resample(AVFrame* indata, unsigned char* data);
	FF_Resample();
	~FF_Resample();
protected:
	std::mutex mux_;
	SwrContext* res_ctx_ = nullptr;
	int out_channels_ = 0;
	int out_sample_rate_ = 44100;
	int out_sample_fmt_ = 0;
};

