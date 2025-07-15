#include "ff_video_thread.h"
#include "ff_decode.h"
#include <iostream>
using namespace std;

bool FF_Video_Thread::Open(AVCodecParameters* para, FF_Video_Call* call, int width, int height)
{
	if (!para)return false;
	Clear();

	vmux.lock();
	synpts = 0;
	this->call = call;
	if (call)
	{
		call->Init(width, height);
	}
	vmux.unlock();
	int re = true;
	if (!decode_->Open(para))
	{
		cout << "video FF_Decode open failed!" << endl;
		re = false;
	}

	cout << "FF_Video_Thread::Open :" << re << endl;
	return re;
}

void FF_Video_Thread::run()
{
	while (!is_Exit)
	{
		vmux.lock();
		if (this->isPause)
		{
			vmux.unlock();
			msleep(5);
			continue;
		}
		if (synpts > 0 && synpts < decode_->pts)
		{
			vmux.unlock();
			msleep(1);
			continue;
		}
		AVPacket* pkt = Pop();
		bool re = decode_->Send(pkt);
		if (!re)
		{
			vmux.unlock();
			msleep(1);
			continue;
		}
		while (!is_Exit)
		{
			AVFrame* frame = decode_->Recv();
			if (!frame) break;
			if (call)
			{
				call->Repaint(frame);
			}
		}
		vmux.unlock();
	}
}

FF_Video_Thread::FF_Video_Thread()
{
}


FF_Video_Thread::~FF_Video_Thread()
{
	is_Exit = true;
	wait();
}


void FF_Video_Thread::SetPause(bool isPause)
{
	vmux.lock();
	this->isPause = isPause;
	vmux.unlock();
}

bool FF_Video_Thread::RepaintPts(AVPacket* pkt, long long seekpts)
{
	vmux.lock();
	bool re = decode_->Send(pkt);
	if (!re)
	{
		vmux.unlock();
		return true;
	}
	AVFrame* frame = decode_->Recv();
	if (!frame)
	{
		vmux.unlock();
		return false;
	}

	if (decode_->pts >= seekpts)
	{
		if (call)
			call->Repaint(frame);
		vmux.unlock();
		return true;
	}
	FreeFrame(&frame);
	vmux.unlock();
	return false;
}