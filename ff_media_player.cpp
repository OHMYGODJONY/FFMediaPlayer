#include "ff_media_player.h"
#include "ff_demux_thread.h"
#include <iostream>
#include <QFileDialog>
#include <QMessageBox>
using namespace std;
static FF_Demux_Thread dt;

FF_Media_Player::FF_Media_Player(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    ui.lineEdit->setPlaceholderText("input RTMP URL");
    dt.Start();
    //startTimer(5);
}

FF_Media_Player::~FF_Media_Player()
{
    dt.Close();
}

void FF_Media_Player::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (isFullScreen())
        this->showNormal();
    else
        this->showFullScreen();
}

void FF_Media_Player::resizeEvent(QResizeEvent* e)
{
    ui.pushButton->move(10, this->height() - 100);
    ui.isplay->move(100, this->height() - 100);
    ui.lineEdit->move(10, this->height() - 70);
    ui.rtmp_open->move(330, this->height() - 70);
    ui.video->move(10, 10);
    ui.video->resize(this->width() - 20, (int)((this->width() - 20) / 16 * 9));
}

void FF_Media_Player::timerEvent(QTimerEvent* e)
{
    
}

void FF_Media_Player::on_openButton()
{
    QString filePath = QFileDialog::getOpenFileName(this, "open file");
    if (filePath.isEmpty())return;
    cout << filePath.toLocal8Bit() << endl;
    std::string file = filePath.toStdString();
    if (!dt.Open(file.c_str(), ui.video))
    {
        QMessageBox::information(0, "error", "open file failed!");
        return;
    }
    SetPause(dt.isPause);
}

void FF_Media_Player::on_rtmpButton()
{
    QString filePath = ui.lineEdit->text();
    if (filePath.isEmpty())return;
    cout << filePath.toLocal8Bit() << endl;
    std::string file = filePath.toStdString();
    if (!dt.Open(file.c_str(), ui.video))//   rtmp://liteavapp.qcloud.com/live/liteavdemoplayerstreamid
    {
        QMessageBox::information(0, "error", "open file failed!");
        return;
    }
    SetPause(dt.isPause);
}

void FF_Media_Player::PlayOrPause()
{
    bool isPause = !dt.isPause;
    SetPause(isPause);
    dt.SetPause(isPause);
}

void FF_Media_Player::SetPause(bool isPause)
{
    if (isPause)
        ui.isplay->setText("play");
    else
        ui.isplay->setText("stop");
}