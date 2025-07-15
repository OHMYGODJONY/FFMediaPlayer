#pragma once
#include <QThread>
#include "ff_video_call.h"
#include <mutex>
class FF_Demux;
class FF_Video_Thread;
class FF_Audio_Thread;
class FF_Demux_Thread :public QThread
{
public:
	virtual bool Open(const char* url, FF_Video_Call* call);
	virtual void Start();
	virtual void Close();
	virtual void Clear();
	virtual void Seek(double pos);

	void run();
	FF_Demux_Thread();
	virtual ~FF_Demux_Thread();

	bool isExit = false;
	long long pts = 0;
	long long totalMs = 0;
	bool isPause = false;
	void SetPause(bool isPause);
protected:
	std::mutex mux;
	FF_Demux* demux = nullptr;
	FF_Video_Thread* vt = nullptr;
	FF_Audio_Thread* at = nullptr;
};

