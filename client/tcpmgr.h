#ifndef TCPMGR_H
#define TCPMGR_H
#include <QObject>
#include <QTcpSocket>
#include "global.h"
#include "singleton.h"
#include "userdata.h"
class TcpMgr : public QObject,public Singleton<TcpMgr>
{
    Q_OBJECT
public:
    ~TcpMgr();
private:
    friend class Singleton<TcpMgr>;
    TcpMgr();
    void initHandlers();
    void handlerMsg(ReqId id,int len,QByteArray data);
    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;
    QMap<ReqId,std::function<void(ReqId id,int len,QByteArray data)>>_handlers;

public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqId reqId, QByteArray data);
signals:
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId reqId, QByteArray data);
    void sig_login_ok();
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo>);

    void sig_friend_apply(std::shared_ptr<AddFriendApply>);//对方申请
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);//收到对方同意消息
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);//别人申请我 我同意了 发送该信号
    void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);//我收到别人给我发的消息
//    void sig_notify_offline();
//    void sig_connection_closed();

};

#endif // TCPMGR_H
