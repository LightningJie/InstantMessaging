#include "timerbtn.h"
#include <QMouseEvent>
#include <QDebug>
TimerBtn::TimerBtn(QWidget *parent):QPushButton(parent),_counter(10)
{
    _timer=new QTimer(this);
    connect(_timer,&QTimer::timeout,[this](){
        _counter--;
        if(_counter<=0){
            _timer->stop();
            _counter=10;
            this->setText("获取");
            this->setEnabled(true);
            return;
        }
        this->setText(QString::number(_counter));
    });
}

TimerBtn::~TimerBtn()
{
    _timer->stop();

}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        // 在这里处理鼠标左键释放事件
        qDebug() << "MyButton was released!";
        this->setEnabled(false);
        this->setText(QString::number(_counter));
        _timer->start(1000);//每 1000 毫秒（即 1 秒）触发一次 timeout 信号
        emit clicked();//普通 QPushButton 在 setEnabled(false) 后不会自动 emit clicked()，所以这里手动 emit 是为了确保逻辑能触发。
    }
    // 调用基类的mouseReleaseEvent以确保正常的事件处理（如点击效果）
    QPushButton::mouseReleaseEvent(e);
}
