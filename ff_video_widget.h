#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <mutex>
#include "ff_video_call.h"

struct AVFrame;

class FF_Video_Widget : public QOpenGLWidget, protected QOpenGLFunctions, public FF_Video_Call
{
    Q_OBJECT

public:
    explicit FF_Video_Widget(QWidget* parent = nullptr);
    ~FF_Video_Widget();

	void Init(int width, int height);
	virtual void Repaint(AVFrame* frame);

protected:
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();
private:
	std::mutex mux;
	QOpenGLExtraFunctions* f;
	QOpenGLShaderProgram program;
	GLuint unis[3] = { 0 };
	GLuint texs[3] = { 0 };
	unsigned char* datas[3] = { 0 };

	int width = 240;
	int height = 128;
};
 
