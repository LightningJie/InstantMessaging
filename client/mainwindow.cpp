#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include "tcpmgr.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);
    _ui_status=LOGIN_UI;
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接创建聊天界面信号
    connect(_login_dlg, &LoginDialog::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
    //链接服务器踢人消息 服务器发送踢人消息，客户端确认下线 断开socket后也会触发sig_connection_closed
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_notify_offline, this, &MainWindow::SlotOffline);
    //连接心跳断开消息 服务器监测心跳失效，断开socket ，触发sig_connection_closed 信号
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_connection_closed, this, &MainWindow::SlotExcepOffline);
}
void MainWindow::SlotSwitchReg()
{
    _reg_dlg = new RegisterDialog(this);
    //_reg_dlg->hide();

    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

     //连接注册界面返回登录信号
    connect(_reg_dlg, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    _reg_dlg->show();
    _ui_status = REGISTER_UI;
}
//从注册界面返回登录界面
void MainWindow::SlotSwitchLogin()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

   _reg_dlg->hide();
    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接创建聊天界面信号
    connect(_login_dlg, &LoginDialog::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
    _ui_status = LOGIN_UI;
}

void MainWindow::SlotSwitchReset()
{
    _ui_status = RESET_UI;
    _reset_dlg = new ResetDialog(this);
    _reset_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_reset_dlg);

    _login_dlg->hide();
    _reset_dlg->show();
    //注册返回登录信号和槽函数
    connect(_reset_dlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
    _ui_status = LOGIN_UI;

}

void MainWindow::SlotSwitchLogin2()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    _reset_dlg->hide();
    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接创建聊天界面信号
    connect(_login_dlg, &LoginDialog::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
    _ui_status = LOGIN_UI;
}

void MainWindow::SlotSwitchChat()
{
    _chat_dlg=new ChatDialog();
    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_chat_dlg);
    _chat_dlg->show();
    _login_dlg->hide();
    _ui_status = CHAT_UI;
    this->setMinimumSize(QSize(1050,900));
    this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    _chat_dlg->loadChatList();
}

void MainWindow::SlotOffline()
{
    // 使用静态方法直接弹出一个信息框
    QMessageBox::information(this, "下线提示", "同账号异地登录，该终端下线！");
    TcpMgr::GetInstance()->CloseConnection();
    //FileTcpMgr::GetInstance()->CloseConnection();
    offlineLogin();
}
void MainWindow::SlotExcepOffline()
{
    // 使用静态方法直接弹出一个信息框
    QMessageBox::information(this, "下线提示", "心跳超时断开连接！");
    TcpMgr::GetInstance()->CloseConnection();
    //FileTcpMgr::GetInstance()->CloseConnection();
    offlineLogin();
}
void MainWindow::offlineLogin()
{
    if(_ui_status == LOGIN_UI){
        return;
    }
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    _chat_dlg->hide();
    this->setMaximumSize(300,500);
    this->setMinimumSize(300,500);
    this->resize(300,500);
    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接创建聊天界面信号
    connect(_login_dlg, &LoginDialog::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
    _ui_status = LOGIN_UI;
}

MainWindow::~MainWindow()
{
    delete ui;

}


