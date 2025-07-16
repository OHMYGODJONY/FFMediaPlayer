#pragma once
class FF_Audio_Play
{
public:
	int sampleRate = 44100;
	int sampleSize = 16;
	int channels = 2;

	virtual bool Open() = 0;
	virtual void Close() = 0;
	virtual void Clear() = 0;
	virtual long long GetNoPlayMs() = 0;
	virtual bool Write(const unsigned char* data, int datasize) = 0;
	virtual int GetFree() = 0;
	virtual void SetPause(bool isPause) = 0;

	static FF_Audio_Play* Get();
	FF_Audio_Play();
	virtual ~FF_Audio_Play();
};

