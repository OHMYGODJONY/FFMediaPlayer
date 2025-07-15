#include "ff_audio_thread.h"
#include "ff_decode.h"
#include "ff_audio_play.h"
#include "ff_resample.h"
#include <iostream>
using namespace std;

void FF_Audio_Thread::Clear()
{
	FF_Decode_List::Clear();
	mux_.lock();
	if (ap) ap->Clear();
	mux_.unlock();
}

void FF_Audio_Thread::Close()
{
	FF_Decode_List::Close();
	if (res)
	{
		res->Close();
		amux.lock();
		delete res;
		res = NULL;
		amux.unlock();
	}
	if (ap)
	{
		ap->Close();
		amux.lock();
		ap = NULL;
		amux.unlock();
	}
}

bool FF_Audio_Thread::Open(AVCodecParameters* para, int sampleRate, int channels)
{
	if (!para)return false;
	Clear();

	amux.lock();
	pts = 0;
	bool re = true;
	if (!res->Open(para, false))
	{
		cout << "FF_Resample open failed!" << endl;
		re = false;
	}
	ap->sampleRate = sampleRate;
	ap->channels = channels;
	if (!ap->Open())
	{
		re = false;
		cout << "FF_Audio_Play open failed!" << endl;
	}
	if (!decode_->Open(para))
	{
		cout << "audio FF_Decode open failed!" << endl;
		re = false;
	}
	amux.unlock();
	cout << "FF_Audio_Thread::Open :" << re << endl;
	return re;
}
void FF_Audio_Thread::run()
{
	unsigned char* pcm = new unsigned char[1024 * 1024 * 10];
	while (!is_Exit)
	{
		amux.lock();
		if (isPause)
		{
			amux.unlock();
			msleep(5);
			continue;
		}
		AVPacket* pkt = Pop();
		bool re = decode_->Send(pkt);
		if (!re)
		{
			amux.unlock();
			msleep(1);
			continue;
		}

		while (!is_Exit)
		{
			AVFrame* frame = decode_->Recv();
			if (!frame) break;

			pts = decode_->pts - ap->GetNoPlayMs();
			int size = res->Resample(frame, pcm);
			while (!is_Exit)
			{
				if (size <= 0)break;
				if (ap->GetFree() < size || isPause)
				{
					msleep(1);
					continue;
				}
				ap->Write(pcm, size);
				break;
			}
		}
		amux.unlock();
	}
	delete [] pcm;
}

FF_Audio_Thread::FF_Audio_Thread()
{
	if (!res) res = new FF_Resample();
	if (!ap) ap = FF_Audio_Play::Get();
}


FF_Audio_Thread::~FF_Audio_Thread()
{
	is_Exit = true;
	wait();
}

void FF_Audio_Thread::SetPause(bool isPause)
{
	this->isPause = isPause;
	if (ap)
	{
		ap->SetPause(isPause);
	}
}