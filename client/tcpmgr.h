#ifndef TCPMGR_H
#define TCPMGR_H
#include <QObject>
#include <QTcpSocket>
#include "global.h"
#include "singleton.h"
#include "userdata.h"
#include <QQueue>
class TcpThread:public std::enable_shared_from_this<TcpThread> {
public:
    TcpThread();
    ~TcpThread();
private:
    QThread* _tcp_thread;
};

class TcpMgr : public QObject,public Singleton<TcpMgr>
{
    Q_OBJECT
public:
    ~TcpMgr();
    void CloseConnection();
    void SendData(ReqId reqId, QByteArray data);
private:   
    friend class Singleton<TcpMgr>;
    void registerMetaType();
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
    //发送队列
    QQueue<QByteArray> _send_queue;
    //正在发送的包
    QByteArray  _current_block;
    //当前已发送的字节数
    qint64        _bytes_sent;
    //是否正在发送
    bool _pending;

public slots:
    void slot_tcp_connect(std::shared_ptr<ServerInfo> si);
    void slot_send_data(ReqId reqId, QByteArray data);
    void slot_tcp_close();
signals:
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId reqId, QByteArray data);
    void sig_login_ok();
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo>);

    void sig_friend_apply(std::shared_ptr<AddFriendApply>);//对方申请
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);//收到对方同意消息
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);//别人申请我 我同意了 发送该信号
    void sig_text_chat_msg(std::vector<std::shared_ptr<TextChatData>> msg_list);//我收到别人给我发的消息
    void sig_notify_offline();//踢人
    void sig_connection_closed();//心跳给客户端关了
    void sig_close();
    void sig_load_chat_thread(bool load_more, int last_thread_id,
            std::vector<std::shared_ptr<ChatThreadInfo>> chat_list);
    void sig_create_private_chat(int uid, int other_id, int thread_id);
    void sig_load_chat_msg(int thread_id, int message_id, bool load_more,
        std::vector<std::shared_ptr<TextChatData>> msg_list);

    void sig_chat_msg_rsp(int thread_id, std::vector<std::shared_ptr<TextChatData>> msg_list);
//    void sig_chat_img_rsp(int thread_id, std::shared_ptr<ImgChatData> msg_list);
//    void sig_img_chat_msg(std::shared_ptr<ImgChatData> msg_list);

};

#endif // TCPMGR_H
