#include "ff_media_player.h"
#include "ff_demux.h"
#include "ff_decode.h"
#include "ff_resample.h"
#include "ff_audio_play.h"
#include "ff_audio_thread.h"
#include "ff_video_thread.h"
#include <iostream>
#include <QThread>
using namespace std;
#include <QtWidgets/QApplication>

#include "ff_demux_thread.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FF_Media_Player w;
    w.show();

    return a.exec();
}
