#pragma once
#include "ff_decode_list.h"
#include "ff_video_call.h"
#include <list>
#include <mutex>
#include <QThread>
struct AVPacket;
struct AVCodecParameters;
class FF_Decode;

class FF_Video_Thread :public FF_Decode_List
{
public:
	virtual bool RepaintPts(AVPacket* pkt, long long seekpts);
	virtual bool Open(AVCodecParameters* para, FF_Video_Call* call, int width, int height);
	void run();

	FF_Video_Thread();
	virtual ~FF_Video_Thread();
	long long synpts = 0;

	void SetPause(bool isPause);
	bool isPause = false;

protected:
	std::mutex vmux;
	FF_Video_Call* call = nullptr;


};