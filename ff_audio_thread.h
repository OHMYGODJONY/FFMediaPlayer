#pragma once
#include "ff_decode_list.h"
#include <QThread>
#include <mutex>
#include <list>
struct AVCodecParameters;
class FF_Audio_Play;
class FF_Resample;


class FF_Audio_Thread :
	public FF_Decode_List
{
public:
	virtual bool Open(AVCodecParameters* para, int sampleRate, int channels);
	virtual void Close();
	virtual void Clear();
	void run();
	FF_Audio_Thread();
	virtual ~FF_Audio_Thread();

	long long pts = 0;

	void SetPause(bool isPause);
	bool isPause = false;
protected:
	std::mutex amux;
	FF_Audio_Play* ap = 0;
	FF_Resample* res = 0;
};
