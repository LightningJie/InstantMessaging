#ifndef CHATUSERLIST_H
#define CHATUSERLIST_H

#include <QListWidget>
#include <QWheelEvent>//滚轮事间
#include <QEvent>
#include <QScrollBar>//真正执行“滚动位置变化”的滚动条控件
#include <QDebug>
class ChatUserList :public QListWidget
{
    Q_OBJECT
public:
    ChatUserList(QWidget *parent=nullptr);
protected:
    bool eventFilter(QObject *watched, QEvent *event)override;//想实现某些功能，但是基类虚函数不提供 就自定义

private:
    bool _load_pending;
signals:
    void sig_loading_chat_user();
};

#endif // CHATUSERLIST_H
