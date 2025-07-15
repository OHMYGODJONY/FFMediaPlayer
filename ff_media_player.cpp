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
    //ui.playPos->move(50, this->height() - 100);
    //ui.playPos->resize(this->width() - 100, ui.playPos->height());
    //ui.openFile->move(100, this->height() - 150);
   /* ui.video->resize(this->size());*/
}

void FF_Media_Player::timerEvent(QTimerEvent* e)
{
    long long total = dt.totalMs;
    if (total > 0)
    {
        double pos = (double)dt.pts / (double)total;
        /*int v = ui.playPos->maximum() * pos;
        ui.playPos->setValue(v);*/
    }
}

void FF_Media_Player::on_openButton()
{
    QString filePath = QFileDialog::getOpenFileName(this, "open file");
    if (filePath.isEmpty())return;
    cout << filePath.toLocal8Bit() << endl;
    if (!dt.Open(filePath.toLocal8Bit(), ui.video))
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