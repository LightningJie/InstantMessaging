#include "tcpmgr.h"
#include <QDebug>
#include <QJsonDocument>
#include <QIODevice>
#include  "usermgr.h"
TcpMgr::~TcpMgr()
{

}

TcpMgr::TcpMgr():_host(""),_port(0),_b_recv_pending(false),_message_id(0),_message_len(0)
{
    QObject::connect(&_socket,&QTcpSocket::connected,[&](){
        qDebug()<<"Connected to server!";
        // 连接建立后发送消息
        emit sig_con_success(true);
    });

    QObject::connect(&_socket,&QTcpSocket::readyRead,[&](){
        _buffer.append(_socket.readAll());

        QDataStream stream(&_buffer, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_5_0);

        forever{
            if(!_b_recv_pending){
                if(_buffer.size()<static_cast<int>(sizeof (quint16)*2)){
                    return;
                }
                // 预读取消息ID和消息长度，但不从缓冲区中移除
                stream >> _message_id >> _message_len;
                _buffer=_buffer.mid(sizeof(quint16)*2);
                qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;
            }

            if(_buffer.size()<_message_len){
                _b_recv_pending=true;
                return;
            }
            _b_recv_pending=false;
            QByteArray data=_buffer.mid(0,_message_len);
            qDebug() << "receive body msg is " << data ;
            _buffer=_buffer.mid(_message_len);
            handlerMsg(ReqId(_message_id),_message_len,data);
        }

    });
    // 处理错误（适用于Qt 5.15之前的版本）
    QObject::connect(&_socket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),
                        [&](QTcpSocket::SocketError socketError) {
           qDebug() << "Error:" << _socket.errorString() ;
           switch (socketError) {
               case QTcpSocket::ConnectionRefusedError:
                   qDebug() << "Connection Refused!";
                   emit sig_con_success(false);
                   break;
               case QTcpSocket::RemoteHostClosedError:
                   qDebug() << "Remote Host Closed Connection!";
                   break;
               case QTcpSocket::HostNotFoundError:
                   qDebug() << "Host Not Found!";
                   emit sig_con_success(false);
                   break;
               case QTcpSocket::SocketTimeoutError:
                   qDebug() << "Connection Timeout!";
                   emit sig_con_success(false);
                   break;
               case QTcpSocket::NetworkError:
                   qDebug() << "Network Error!";
                   break;
               default:
                   qDebug() << "Other Error!";
                   break;
           }
     });
    // 处理连接断开
    QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
        qDebug() << "Disconnected from server.";
    });

    QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);
    initHandlers();
}

void TcpMgr::initHandlers()
{
    //登录请求的回包
    _handlers.insert(ReqId::ID_CHAT_LOGIN_RSP,[this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug()<<"handle id is"<<id<<"  data is  "<<data;
        QJsonDocument jsonDoc=QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDoc";
            return;
        }
        QJsonObject jsonObj=jsonDoc.object();

        if(!jsonObj.contains("error")){
            int err=ErrorCodes::ERR_JSON;
            qDebug()<<"Login Failed, err is Json Parse Err"<<err;
            emit sig_login_failed(err);
        }
        int err=jsonObj["error"].toInt();
        if(err!=ErrorCodes::SUCCESS){
            qDebug()<<"Login Failed ,err is"<<err;
            emit sig_login_failed(err);
            return;
        }

        auto name =jsonObj["name"].toString();
        auto uid=jsonObj["uid"].toInt();
        auto icon=jsonObj["icon"].toString();
        auto nick=jsonObj["nick"].toString();
        auto sex=jsonObj["sex"].toInt();


        auto user_info=std::make_shared<UserInfo>(uid,name,nick,icon,sex);
        UserMgr::GetInstance()->SetUserInfo(user_info);
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());

        if(jsonObj.contains("apply_list")){
            UserMgr::GetInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }
        //添加好友列表
        if(jsonObj.contains("friend_list")){
            UserMgr::GetInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }
        emit sig_login_ok();
    });
    //查询好友的回包
    _handlers.insert(ReqId::ID_SEARCH_USER_RSP,[this](ReqId id, int len, QByteArray data){

        Q_UNUSED(len);
        qDebug()<<"ReqId::ID_SEARCH_USER_RSP handle id is"<<id<<"  data is  "<<data;
        QJsonDocument jsonDoc=QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDoc";
            return;
        }
        QJsonObject jsonObj=jsonDoc.object();

        if(!jsonObj.contains("error")){
            int err=ErrorCodes::ERR_JSON;
            qDebug()<<"Add friend Failed, err is Json Parse Err"<<err;
            emit sig_user_search(nullptr);
            return;
        }
        int err=jsonObj["error"].toInt();
        if(err!=ErrorCodes::SUCCESS){
            qDebug()<<"add friend Failed ,err is"<<err;
            emit sig_user_search(nullptr);
            return;
        }
        auto search_info =std::make_shared<SearchInfo>(jsonObj["uid"].toInt(),jsonObj["name"].toString(),
                jsonObj["nick"].toString(),jsonObj["des"].toString(),jsonObj["sex"].toInt(),jsonObj["icon"].toString());


        emit sig_user_search(search_info);
    });
    //我发送添加好友请求的回包
    _handlers.insert(ReqId::ID_ADD_FRIEND_RSP,[this](ReqId id, int len, QByteArray data){

        Q_UNUSED(len);
        qDebug()<<"handle id is"<<id<<"  data is  "<<data;
        QJsonDocument jsonDoc=QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDoc";
            return;
        }
        QJsonObject jsonObj=jsonDoc.object();

        if(!jsonObj.contains("error")){
            int err=ErrorCodes::ERR_JSON;
            qDebug()<<"Add friend Failed, err is Json Parse Err"<<err;
        }
        int err=jsonObj["error"].toInt();
        if(err!=ErrorCodes::SUCCESS){
            qDebug()<<"add friend Failed ,err is"<<err;
            return;
        }
        qDebug()<<"add friend req success";
    });
    //别人添加我为好友的请求
    _handlers.insert(ReqId::ID_NOTIFY_ADD_FRIEND_REQ,[this](ReqId id, int len, QByteArray data){

        Q_UNUSED(len);
        qDebug()<<"handle id is"<<id<<"  data is  "<<data;
        QJsonDocument jsonDoc=QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<<"ID_NOTIFY_ADD_FRIEND_REQ failed to create QJsonDoc";
            return;
        }
        QJsonObject jsonObj=jsonDoc.object();

        if(!jsonObj.contains("error")){
            int err=ErrorCodes::ERR_JSON;
            qDebug()<<"ID_NOTIFY_ADD_FRIEND_REQ Failed, err is Json Parse Err"<<err;
            emit sig_login_failed(err);
        }
        int err=jsonObj["error"].toInt();
        if(err!=ErrorCodes::SUCCESS){
            qDebug()<<"ID_NOTIFY_ADD_FRIEND_REQ Failed ,err is"<<err;
            return;
        }
        int from_uid=jsonObj["applyuid"].toInt();
        QString name =jsonObj["name"].toString();
        QString desc =jsonObj["desc"].toString();
        QString icon=jsonObj["icon"].toString();
        QString nick=jsonObj["nick"].toString();

        int sex=jsonObj["sex"].toInt();

        auto apply_info =std::make_shared<AddFriendApply>(from_uid,name,desc,icon,nick,sex);

        emit sig_friend_apply(apply_info);
        qDebug()<<"ID_NOTIFY_ADD_FRIEND_REQ success";
    });
    //我给他人发送好友申请，对方好友认证通过了服务器给我发的请求
    _handlers.insert(ReqId::ID_NOTIFY_AUTH_FRIEND_REQ,[this](ReqId id, int len, QByteArray data){

        Q_UNUSED(len);
        qDebug()<<"handle id is"<<id<<"  data is  "<<data;
        QJsonDocument jsonDoc=QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<<"ID_NOTIFY_AUTH_FRIEND_REQ failed to create QJsonDoc";
            return;
        }
        QJsonObject jsonObj=jsonDoc.object();

        if(!jsonObj.contains("error")){
            int err=ErrorCodes::ERR_JSON;
            qDebug()<<"ID_NOTIFY_AUTH_FRIEND_REQ Failed, err is Json Parse Err"<<err;
            emit sig_login_failed(err);
            return;
        }
        int err=jsonObj["error"].toInt();
        if(err!=ErrorCodes::SUCCESS){
            qDebug()<<"ID_NOTIFY_AUTH_FRIEND_REQ Failed ,err is"<<err;
            return;
        }
        int from_uid=jsonObj["fromuid"].toInt();
        QString name =jsonObj["name"].toString();
        QString icon=jsonObj["icon"].toString();
        QString nick=jsonObj["nick"].toString();
        int sex=jsonObj["sex"].toInt();

        auto auth_info =std::make_shared<AuthInfo>(from_uid,name,nick,icon,sex);

        emit sig_add_auth_friend(auth_info);
        qDebug()<<"ID_NOTIFY_AUTH_FRIEND_REQ success";
    });
    //我发送好友认证通过请求后，服务器给我的回包
    _handlers.insert(ReqId::ID_AUTH_FRIEND_RSP,[this](ReqId id, int len, QByteArray data){

        Q_UNUSED(len);
        qDebug()<<"handle id is"<<id<<"  data is  "<<data;
        QJsonDocument jsonDoc=QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<<"ID_AUTH_FRIEND_RSP failed to create QJsonDoc";
            return;
        }
        QJsonObject jsonObj=jsonDoc.object();

        if(!jsonObj.contains("error")){
            int err=ErrorCodes::ERR_JSON;
            qDebug()<<"ID_AUTH_FRIEND_RSP Failed, err is Json Parse Err"<<err;
            emit sig_login_failed(err);
            return;
        }
        int err=jsonObj["error"].toInt();
        if(err!=ErrorCodes::SUCCESS){
            qDebug()<<"ID_AUTH_FRIEND_RSP Failed ,err is"<<err;
            return;
        }
        //对方的信息
        int uid=jsonObj["uid"].toInt();
        QString name =jsonObj["name"].toString();
        QString icon=jsonObj["icon"].toString();
        QString nick=jsonObj["nick"].toString();
        int sex=jsonObj["sex"].toInt();

        auto rsp =std::make_shared<AuthRsp>(uid,name,nick,icon,sex);

        emit sig_auth_rsp(rsp);
        qDebug()<<"ID_AUTH_FRIEND_RSP success";
    });
    //自己发完消息的服务器回包
    _handlers.insert(ID_TEXT_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Chat Msg Rsp Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Chat Msg Rsp Failed, err is " << err;
            return;
        }

        qDebug() << "Receive Text Chat Rsp Success " ;
        //ui设置送达等标记 todo...
      });
    //收到别人给我发送的消息请求
    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "handle id is " << id << " data is " << data;
            // 将QByteArray转换为QJsonDocument
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

            // 检查转换是否成功
            if (jsonDoc.isNull()) {
                qDebug() << "Failed to create QJsonDocument.";
                return;
            }

            QJsonObject jsonObj = jsonDoc.object();

            if (!jsonObj.contains("error")) {
                int err = ErrorCodes::ERR_JSON;
                qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
                return;
            }

            int err = jsonObj["error"].toInt();
            if (err != ErrorCodes::SUCCESS) {
                qDebug() << "Notify Chat Msg Failed, err is " << err;
                return;
            }

            qDebug() << "Receive Text Chat Notify Success " ;
            auto msg_ptr = std::make_shared<TextChatMsg>(jsonObj["fromuid"].toInt(),
                    jsonObj["touid"].toInt(),jsonObj["text_array"].toArray());
            emit sig_text_chat_msg(msg_ptr);
          });

//    _handlers.insert(ID_NOTIFY_OFF_LINE_REQ,[this](ReqId id, int len, QByteArray data){
//        Q_UNUSED(len);
//        qDebug() << "handle id is " << id << " data is " << data;
//        // 将QByteArray转换为QJsonDocument
//        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

//        // 检查转换是否成功
//        if (jsonDoc.isNull()) {
//            qDebug() << "Failed to create QJsonDocument.";
//            return;
//        }

//        QJsonObject jsonObj = jsonDoc.object();

//        if (!jsonObj.contains("error")) {
//            int err = ErrorCodes::ERR_JSON;
//            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
//            return;
//        }

//        int err = jsonObj["error"].toInt();
//        if (err != ErrorCodes::SUCCESS) {
//            qDebug() << "Notify Chat Msg Failed, err is " << err;
//            return;
//        }

//        auto uid = jsonObj["uid"].toInt();
//        qDebug() << "Receive offline Notify Success, uid is " << uid ;
//        //断开连接
//        //并且发送通知到界面
//        emit sig_notify_offline();
//    });
}

void TcpMgr::handlerMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter=_handlers.find(id);
    if(find_iter==_handlers.end()){
        qDebug()<<"not find in handlers  id is :"<<id;
        return;
    }
    find_iter.value()(id,len,data);
}

void TcpMgr::slot_tcp_connect(ServerInfo s)
{
    qDebug()<< "receive tcp connect signal";
    // 尝试连接到服务器
    qDebug() << "Connecting to server...";
    _host=s.Host;
    _port = static_cast<uint16_t>(s.Port.toUInt());
    _socket.connectToHost(s.Host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QByteArray data)
{
    uint16_t id=reqId;
    quint16 len=static_cast<uint16_t>(data.size());
    QByteArray block;
    QDataStream out(&block,QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out<<id<<len;
    block.append(data);
    _socket.write(block);
}


