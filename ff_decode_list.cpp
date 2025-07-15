#include "ff_decode_list.h"
#include "ff_decode.h"
void FF_Decode_List::Close()
{
	Clear();
	is_Exit = true;
	wait();
	decode_->Close();

	mux_.lock();
	delete decode_;
	decode_ = NULL;
	mux_.unlock();
}

void FF_Decode_List::Clear()
{
	mux_.lock();
	decode_->Clear();
	while (!packs_.empty())
	{
		AVPacket* pkt = packs_.front();
		FreePacket(&pkt);
		packs_.pop_front();
	}
	mux_.unlock();
}

AVPacket* FF_Decode_List::Pop()
{
	mux_.lock();
	if (packs_.empty())
	{
		mux_.unlock();
		return NULL;
	}
	AVPacket* pkt = packs_.front();
	packs_.pop_front();
	mux_.unlock();
	return pkt;
}

void FF_Decode_List::Push(AVPacket* pkt)
{
	if (!pkt) return;
	while (!is_Exit)
	{
		mux_.lock();
		if (packs_.size() < maxList)
		{
			packs_.push_back(pkt);
			mux_.unlock();
			break;
		}
		mux_.unlock();
		msleep(1);
	}
}

FF_Decode_List::FF_Decode_List()
{
	if (!decode_) decode_ = new FF_Decode();
}


FF_Decode_List::~FF_Decode_List()
{
	is_Exit = true;
	wait();
}
