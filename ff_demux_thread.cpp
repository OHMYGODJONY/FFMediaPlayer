#include "ff_demux_thread.h"
#include "ff_demux.h"
#include "ff_video_thread.h"
#include "ff_audio_thread.h"
#include <iostream>
using namespace std;

void FF_Demux_Thread::Clear()
{
	mux.lock();
	if (demux)demux->Clear();
	if (vt) vt->Clear();
	if (at) at->Clear();
	mux.unlock();
}

void FF_Demux_Thread::Seek(double pos)
{
	Clear();
	mux.lock();
	bool status = this->isPause;
	mux.unlock();
	SetPause(true);

	mux.lock();
	long long seekPts = 0;
	if (demux)
	{
		demux->Seek(pos);
		long long seekPts = pos * (demux->total_ms);
	}

	while (!isExit)
	{
		AVPacket* pkt = demux->Read_Video();
		if (!pkt) break;
		if (vt->RepaintPts(pkt, seekPts))
		{
			this->pts = seekPts;
			break;
		}
	}
	mux.unlock();
	if (!status)
		SetPause(false);
}

void FF_Demux_Thread::run()
{
	while (!isExit)
	{
		mux.lock();
		if (isPause)
		{
			mux.unlock();
			msleep(5);
			continue;
		}
		if (!demux)
		{
			mux.unlock();
			msleep(5);
			continue;
		}

		if (vt && at)
		{
			pts = at->pts;
			vt->synpts = at->pts;
		}

		AVPacket* pkt = demux->Read();
		if (!pkt)
		{
			mux.unlock();
			msleep(5);
			continue;
		}
		if (demux->Is_Audio(pkt))
		{
			if (at) 
			{
				at->Push(pkt);
			}
			msleep(1);
		}
		else
		{
			if (vt) 
			{
				vt->Push(pkt);
			}
			msleep(5);
		}
		mux.unlock();
	}
}

bool FF_Demux_Thread::Open(const char* url, FF_Video_Call* call)
{
	if (url == 0 || url[0] == '\0')
	{
		return false;
	}

	mux.lock();
	if (!demux) demux = new FF_Demux();
	if (!vt) vt = new FF_Video_Thread();
	if (!at) at = new FF_Audio_Thread();

	bool re = demux->Open(url);
	if (!re)
	{
		mux.unlock();
		cout << "demux->Open(url) failed!" << endl;
		return false;
	}

	if (!vt->Open(demux->Video_Para(), call, demux->width, demux->height))
	{
		re = false;
		cout << "vt->Open failed!" << endl;
	}

	if (!at->Open(demux->Audio_Para(), demux->sample_rate, demux->channels))
	{
		re = false;
		cout << "at->Open failed!" << endl;
	}
	totalMs = demux->total_ms;
	mux.unlock();
	cout << "FF_Demux_Thread::Open " << re << endl;
	return re;
}

void FF_Demux_Thread::Close()
{
	isExit = true;
	wait();
	if (vt) vt->Close();
	if (at) at->Close();
	mux.lock();
	delete vt;
	delete at;
	vt = NULL;
	at = NULL;
	mux.unlock();
}

void FF_Demux_Thread::Start()
{
	mux.lock();
	if (!demux) demux = new FF_Demux();
	if (!vt) vt = new FF_Video_Thread();
	if (!at) at = new FF_Audio_Thread();
	QThread::start();
	if (vt)vt->start();
	if (at)at->start();
	mux.unlock();
}
FF_Demux_Thread::FF_Demux_Thread()
{
}


FF_Demux_Thread::~FF_Demux_Thread()
{
	isExit = true;
	wait();
}

void FF_Demux_Thread::SetPause(bool isPause)
{
	mux.lock();
	this->isPause = isPause;
	if (at) at->SetPause(isPause);
	if (vt) vt->SetPause(isPause);
	mux.unlock();
}