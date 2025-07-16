#include "ff_resample.h"
extern "C" {
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
}
#pragma comment(lib,"swresample.lib")
#include <iostream>
using namespace std;

void FF_Resample::Close()
{
	mux_.lock();
	if (res_ctx_)
	{
		swr_free(&res_ctx_);
		res_ctx_ = nullptr;
	}
	mux_.unlock();
}

bool FF_Resample::Open(AVCodecParameters* para, bool isClearPara)
{
	if (!para)return false;
	mux_.lock();
	res_ctx_ = swr_alloc_set_opts(res_ctx_,
		av_get_default_channel_layout(para->channels),
		AV_SAMPLE_FMT_S16,
		44100,
		av_get_default_channel_layout(para->channels),
		(AVSampleFormat)para->format,
		para->sample_rate,
		0, 0);

	if (isClearPara)
		avcodec_parameters_free(&para);

	int re = swr_init(res_ctx_);
	mux_.unlock();
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		cout << "swr_init  failed! :" << buf << endl;
		return false;
	}
	return true;
}

int FF_Resample::Resample(AVFrame* indata, unsigned char* d)
{
	if (!indata) return 0;
	if (!d)
	{
		av_frame_free(&indata);
		return 0;
	}
	uint8_t* data[2] = { 0 };
	data[0] = d;
	int re = swr_convert(res_ctx_,
		data, indata->nb_samples,
		(const uint8_t**)indata->data, indata->nb_samples
	);
	int outSize = re * indata->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
	av_frame_free(&indata);
	if (re <= 0)return re;
	return outSize;
}

FF_Resample::FF_Resample() = default;

FF_Resample::~FF_Resample()
{
}