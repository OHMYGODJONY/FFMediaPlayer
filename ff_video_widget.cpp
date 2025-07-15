#include "ff_video_widget.h"
extern "C" {
#include <libavutil/frame.h>
}

#define GET_STR(x) #x
#define A_VER 0
#define T_VER 1

// 顶点shader
const char* vString = GET_STR(
attribute vec4 vertexIn;
attribute vec2 textureIn;
varying vec2 textureOut;
void main(void)
{
    gl_Position = vertexIn;
    textureOut = textureIn;
}
);

// 片元shader
const char* tString = GET_STR(
varying vec2 textureOut;
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;
void main(void)
{
    vec3 yuv;
    vec3 rgb;
    yuv.x = texture2D(tex_y, textureOut).r;
    yuv.y = texture2D(tex_u, textureOut).r - 0.5;
    yuv.z = texture2D(tex_v, textureOut).r - 0.5;
    rgb = mat3(1.0, 1.0, 1.0,
        0.0, -0.39465, 2.03211,
        1.13983, -0.58060, 0.0) * yuv;
    gl_FragColor = vec4(rgb, 1.0);
}
);

FF_Video_Widget::FF_Video_Widget(QWidget* parent)
    : QOpenGLWidget(parent)
{

}

FF_Video_Widget::~FF_Video_Widget()
{
    //if (fp) fclose(fp);
    delete[] datas[0];
    delete[] datas[1];
    delete[] datas[2];
}

void FF_Video_Widget::Repaint(AVFrame* frame)
{
    if (!frame)return;
    mux.lock();
    if (!datas[0] || width * height == 0 || frame->width != this->width || frame->height != this->height)
    {
        av_frame_free(&frame);
        mux.unlock();
        return;
    }
    memcpy(datas[0], frame->data[0], width * height);
    memcpy(datas[1], frame->data[1], width * height / 4);
    memcpy(datas[2], frame->data[2], width * height / 4);
    av_frame_free(&frame);
    mux.unlock();
    update();
}

void FF_Video_Widget::Init(int width, int height)
{
    mux.lock();
    this->width = width;
    this->height = height;
    delete datas[0];
    delete datas[1];
    delete datas[2];
    datas[0] = new quint8[width * height];      // Y
    datas[1] = new quint8[width * height / 4];  // U
    datas[2] = new quint8[width * height / 4];  // V

    if (texs[0])
    {
        f->glDeleteTextures(3, texs);
    }
    // 创建纹理
    f->glGenTextures(3, texs);

    // Y纹理
    f->glBindTexture(GL_TEXTURE_2D, texs[0]);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // U纹理
    f->glBindTexture(GL_TEXTURE_2D, texs[1]);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    // V纹理
    f->glBindTexture(GL_TEXTURE_2D, texs[2]);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    mux.unlock();
}

void FF_Video_Widget::initializeGL()
{
    qDebug() << "initializeGL";
    mux.lock();
    initializeOpenGLFunctions();
    f = QOpenGLContext::currentContext()->extraFunctions();

    // 创建shader程序
    program.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vString);
    program.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, tString);

    if (!program.link()) {
        qDebug() << "Shader program link failed!";
        return;
    }

    if (!program.bind()) {
        qDebug() << "Shader program bind failed!";
        return;
    }

    // 顶点数据
    static const GLfloat ver[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };

    // 纹理坐标
    static const GLfloat tex[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    // 设置顶点属性
    program.enableAttributeArray(A_VER);
    program.setAttributeArray(A_VER, ver, 2);
    program.enableAttributeArray(T_VER);
    program.setAttributeArray(T_VER, tex, 2);

    // 获取uniform位置
    unis[0] = program.uniformLocation("tex_y");
    unis[1] = program.uniformLocation("tex_u");
    unis[2] = program.uniformLocation("tex_v");

    mux.unlock();
}

void FF_Video_Widget::paintGL()
{
    mux.lock();
    program.bind();

    // Y
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, texs[0]);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, datas[0]);
    program.setUniformValue(unis[0], 0);

    // U
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, texs[1]);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, datas[1]);
    program.setUniformValue(unis[1], 1);

    // V
    f->glActiveTexture(GL_TEXTURE2);
    f->glBindTexture(GL_TEXTURE_2D, texs[2]);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, datas[2]);
    program.setUniformValue(unis[2], 2);

    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    mux.unlock();
}

void FF_Video_Widget::resizeGL(int w, int h)
{
    mux.lock();
    qDebug() << "resizeGL " << width << ":" << height;
    mux.unlock();
}
