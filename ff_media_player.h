#pragma once

#include <QtWidgets/QWidget>
#include "ui_ff_media_player.h"

class FF_Media_Player : public QWidget
{
    Q_OBJECT

public:
    FF_Media_Player(QWidget *parent = nullptr);
    ~FF_Media_Player();

    void timerEvent(QTimerEvent* e);
    void resizeEvent(QResizeEvent* e);
    void mouseDoubleClickEvent(QMouseEvent* e);
    void SetPause(bool isPause);

private slots:
    void on_openButton();
    void PlayOrPause();

//private:
public:
    Ui::FF_Media_PlayerClass ui;
    
};
