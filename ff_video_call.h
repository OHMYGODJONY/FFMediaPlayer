#pragma once
struct AVFrame;

class FF_Video_Call
{
public:
	virtual void Init(int width, int height) = 0;
	virtual void Repaint(AVFrame* frame) = 0;
};

