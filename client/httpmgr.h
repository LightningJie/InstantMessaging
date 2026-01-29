#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
//CRTP
class HttpMgr:public QObject,public Singleton<HttpMgr>,public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT
public:
    ~HttpMgr();//提问 为什么析构要是公有？ 答案：析构时候 智能指针调用T内部的析构。
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id,Modules mod);
private:
    friend class Singleton;//你是我朋友 你可以访问我
    HttpMgr();
    QNetworkAccessManager _manager;
private slots:
    void slot_http_finish(ReqId id, QString res,ErrorCodes err,Modules mod);
signals:
    void sig_http_finish(ReqId id, QString res,ErrorCodes err,Modules mod);
    void sig_reg_mod_finish(ReqId id,QString res,ErrorCodes err);
    void sig_reset_mod_finish(ReqId id,QString res,ErrorCodes err);
    void sig_login_mod_finish(ReqId id,QString res,ErrorCodes err);
};

#endif // HTTPMGR_H
