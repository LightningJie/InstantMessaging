#ifndef TIMERBTN_H
#define TIMERBTN_H
#include<QPushButton>
#include<QTimer>

class TimerBtn:public QPushButton
{
public:
    TimerBtn(QWidget *parent = nullptr);
    ~TimerBtn();
    void mouseReleaseEvent(QMouseEvent *e)override;//自定义按钮在鼠标松开时的行为 override 覆盖
private:
    QTimer *_timer;
    int _counter;
};

#endif // TIMERBTN_H
