#pragma once
struct AVPacket;
class FF_Decode;
#include <list>
#include <mutex>
#include <QThread>
class FF_Decode_List:
	public QThread
{
public:
	FF_Decode_List();
	virtual ~FF_Decode_List();
	virtual void Push(AVPacket* pkt);
	virtual void Clear();
	virtual void Close();
	virtual AVPacket* Pop();

	int maxList = 100;
	bool is_Exit = false;
protected:
	FF_Decode* decode_ = nullptr;
	std::list<AVPacket*> packs_;
	std::mutex mux_;
};

