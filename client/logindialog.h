#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private:
    void initHttpHandlers();
    void initHead();
    bool checkUserValid();
    bool checkPwdValid();
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    void showTip(QString str,bool b_ok);
    bool enableBtn(bool enabled);
    Ui::LoginDialog *ui;
    QMap<ReqId,std::function<void(const QJsonObject &)>> _handlers;
    QMap<TipErr, QString> _tip_errs;//错误的历史记录
    int _uid;
    QString _token;
    std::shared_ptr<ServerInfo> _si;
signals:
    void switchRegister();
    void switchReset();
    void sig_connect_tcp(std::shared_ptr<ServerInfo>);
    void sig_swich_chatdlg();
private slots:
    void on_login_btn_clicked();
    void slot_login_mod_finish(ReqId id ,QString res,ErrorCodes err);
    void slot_tcp_con_finish(bool bsuccess);
    void slot_login_failed(int);
    void slot_login_ok();

};

#endif // LOGINDIALOG_H
