#include "chatdialog.h"
#include "ui_chatdialog.h"
#include <QAction>
#include <QRandomGenerator>
#include "chatuserwid.h"
#include "loadingdlg.h"
#include "statewidget.h"
#include "tcpmgr.h"
#include "usermgr.h"
#include "conuseritem.h"
#include <QJsonDocument>
ChatDialog::ChatDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChatDialog),_mode(ChatUIMode::ChatMode),_state(ChatUIMode::ChatMode),
    _b_loading(false),_cur_chat_uid(0),_timer(new QTimer(this))
{
    ui->setupUi(this);
    ui->add_btn->SetState("normal","hover","press");
    ui->search_edit->SetMaxLength(15);

    //创建搜索图片设置搜索文字
    searchAction =new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));
    ui->search_edit->addAction(searchAction,QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));//占位提示文字（placeholder）：当输入框内容为空时，用灰色/浅色显示一段提示

    //创建清除动作 并设置图标
    clearAction =new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));//空白图片
    ui->search_edit->addAction(clearAction,QLineEdit::TrailingPosition);
    connect(ui->search_edit,&QLineEdit::textChanged,[this](const QString &text){
        if(!text.isEmpty()){
            clearAction->setIcon(QIcon(":/res/close_search.png"));//x图片
        }else{
            clearAction->setIcon(QIcon(":/res/close_transparent.png"));//空白图片
        }
    });
    connect(clearAction,&QAction::triggered,[this](){
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/res/close_transparent.png"));//空白图片
        ui->search_edit->clearFocus();
        showSearch(false);
    });
    showSearch(false);
    connect(ui->chat_user_list,&ChatUserList::sig_loading_chat_user,this,&ChatDialog::slot_loading_chat_user);

    QPixmap pixmap(":/res/head_1.jpg");
    QPixmap scaledPixmap =pixmap.scaled(ui->side_head_lb->size(),Qt::KeepAspectRatio);
    ui->side_head_lb->setPixmap(scaledPixmap);
    ui->side_head_lb->setScaledContents(true);//所有内容可伸缩


    ui->side_chat_lb->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    ui->side_contact_lb->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    ui->side_settings_lb->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");

    ui->side_chat_lb->SetSelected(true);
    SetSelectChatItem();
    SetSelectChatPage();

    AddLBGroup(ui->side_chat_lb);
    AddLBGroup(ui->side_contact_lb);
    //AddLBGroup(ui->side_settings_lb);

    connect(ui->side_chat_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(ui->side_contact_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);
    //connect(ui->side_settings_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_setting);

    //链接搜索框输入变化
    connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);
    //检测鼠标点击位置判断是否要清空搜索框
    this->installEventFilter(this); // 安装事件过滤器

    //设置中心部件为chatpage
    ui->stackedWidget->setCurrentWidget(ui->chat_page);
    ui->search_list->SetSearchEdit(ui->search_edit);
    //连接被别人添加好友信号
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_friend_apply,this,&ChatDialog::slot_apply_friend);

    //链接对端同意认证后通知的信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_add_auth_friend,this,
            &ChatDialog::slot_add_auth_friend);

    //链接自己点击同意认证后界面刷新
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_auth_rsp,this,
            &ChatDialog::slot_auth_rsp);

    connect(ui->search_list,&SearchList::sig_jump_chat_item,this,&ChatDialog::slot_jump_chat_item);
    connect(ui->friend_info_page,&FriendInfoPage::sig_jump_chat_item,this,&ChatDialog::slot_jump_chat_item_from_infopage);

    //连接滚动鼠标加载联系人的信号和槽函数
    connect(ui->con_user_list, &ContactUserList::sig_loading_contact_user,
            this, &ChatDialog::slot_loading_contact_user);
    //点击联系人跳转到联系人页面
    connect(ui->con_user_list,&ContactUserList::sig_switch_friend_info_page,
            this,&ChatDialog::slot_friend_info_page);
    //连接联系人页面点击好友申请条目的信号
    connect(ui->con_user_list, &ContactUserList::sig_switch_apply_friend_page,
            this,&ChatDialog::slot_switch_apply_friend_page);
    //连接聊天列表项的点击
    connect(ui->chat_user_list,&QListWidget::itemClicked,this,&ChatDialog::slot_item_clicked);

    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_text_chat_msg,this,&ChatDialog::slot_text_chat_msg);

    connect(_timer, &QTimer::timeout, this, []() {
            auto user_info = UserMgr::GetInstance()->GetUserInfo();
            QJsonObject textObj;
            textObj["fromuid"] = user_info->_uid;
            QJsonDocument doc(textObj);
            QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
            emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_HEART_BEAT_REQ, jsonData);
            });

    _timer->start(10000);//启动定时器 10秒触发1次

    //连接tcp返回的加载聊天回复
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_load_chat_thread,
        this, &ChatDialog::slot_load_chat_thread);

    //连接tcp返回的创建私聊的回复
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_create_private_chat,
        this, &ChatDialog::slot_create_private_chat);

    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_load_chat_msg,
        this, &ChatDialog::slot_load_chat_msg);

    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_chat_msg_rsp, this, &ChatDialog::slot_add_chat_msg);

//	//连接tcp返回的图片聊天信息回复
//	connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_chat_img_rsp, this, &ChatDialog::slot_add_img_msg);
//	//重置label icon
//	connect(FileTcpMgr::GetInstance().get(), &FileTcpMgr::sig_reset_label_icon, this, &ChatDialog::slot_reset_icon);
//	//接收tcp返回的上传进度信息
//	connect(FileTcpMgr::GetInstance().get(), &FileTcpMgr::sig_update_upload_progress,
//		this, &ChatDialog::slot_update_upload_progress);
//	//接收tcp返回的下载进度信息
//	connect(FileTcpMgr::GetInstance().get(), &FileTcpMgr::sig_update_download_progress,
//		this, &ChatDialog::slot_update_download_progress);

//	//接收tcp返回的下载完成信息
//	connect(FileTcpMgr::GetInstance().get(), &FileTcpMgr::sig_download_finish,
//		this, &ChatDialog::slot_download_finish);
}

ChatDialog::~ChatDialog()
{
    _timer->stop();
    delete ui;
}
void ChatDialog::loadChatList()
{
    showLoadingDlg(true);
    //发送请求逻辑
    QJsonObject jsonObj;
    auto uid = UserMgr::GetInstance()->GetUid();
    jsonObj["uid"] = uid;

    int last_chat_thread_id = UserMgr::GetInstance()->GetLastChatThreadId();
    jsonObj["thread_id"] = last_chat_thread_id;


    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //发送tcp请求给chat server
    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_THREAD_REQ, jsonData);
}


void ChatDialog::loadChatMsg() {

    //发送聊天记录请求
    _cur_load_chat = UserMgr::GetInstance()->GetCurLoadData();
    if (_cur_load_chat == nullptr) {
        return;
    }

    showLoadingDlg(true);

    //发送请求给服务器
        //发送请求逻辑
    QJsonObject jsonObj;
    jsonObj["thread_id"] = _cur_load_chat->GetThreadId();
    jsonObj["message_id"] = _cur_load_chat->GetLastMsgId();

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //发送tcp请求给chat server
    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_MSG_REQ, jsonData);
}

bool ChatDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
       QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
       handleGlobalMousePress(mouseEvent);
    }
    return QDialog::eventFilter(watched, event);
}
void ChatDialog::handleGlobalMousePress(QMouseEvent *event)
{
    // 实现点击位置的判断和处理逻辑
    // 先判断是否处于搜索模式，如果不处于搜索模式则直接返回
    if( _mode != ChatUIMode::SearchMode){
        return;
    }

    // 将鼠标点击的全局位置转换为搜索列表坐标系中的位置
    QPoint posInSearchList = ui->search_list->mapFromGlobal(event->globalPos());
    // 判断点击位置是否在聊天列表的范围内
    if (!ui->search_list->rect().contains(posInSearchList)) {
        // 如果不在聊天列表内，清空输入框
        ui->search_edit->clear();
        showSearch(false);
    }
}

void ChatDialog::showLoadingDlg(bool show)
{
    if (show) {
        if (_loading_dlg) {
            _loading_dlg->deleteLater();
        }
        _loading_dlg = new LoadingDlg(this, "正在加载聊天列表...");
        _loading_dlg->setModal(true);
        _loading_dlg->show();
        return;
    }

    if (_loading_dlg) {
        _loading_dlg->deleteLater();
        _loading_dlg = nullptr;
    }

}

void ChatDialog::CloseFindDlg()
{
    ui->search_list->CloseFindDlg();
}


void ChatDialog::ClearLabelState(StateWidget *lb)
{
    for(auto lb_:_lb_list){
        if(lb==lb_){
            continue;
        }
        lb_->ClearState();
    }

}

void ChatDialog::SetSelectChatItem(int thread_id)
{
    if (ui->chat_user_list->count() <= 0) {
        return;
    }

    if (thread_id == 0) {
        ui->chat_user_list->setCurrentRow(0);
        QListWidgetItem* firstItem = ui->chat_user_list->item(0);
        if (!firstItem) {
            return;
        }

        //转为widget
        QWidget* widget = ui->chat_user_list->itemWidget(firstItem);
        if (!widget) {
            return;
        }

        auto con_item = qobject_cast<ChatUserWid*>(widget);
        if (!con_item) {
            return;
        }

        _cur_chat_thread_id = con_item->GetChatData()->GetThreadId();

        return;
    }

    auto find_iter = _chat_thread_items.find(thread_id);
    if (find_iter == _chat_thread_items.end()) {
        qDebug() << "thread_id [" << thread_id << "] not found, set curent row 0";
        ui->chat_user_list->setCurrentRow(0);
        return;
    }

    ui->chat_user_list->setCurrentItem(find_iter.value());

    _cur_chat_thread_id = thread_id;
}

void ChatDialog::SetSelectChatPage(int thread_id)
{
    if (ui->chat_user_list->count() <= 0) {
        return;
    }

    if (thread_id == 0) {
        auto item = ui->chat_user_list->item(0);
        //转为widget
        QWidget* widget = ui->chat_user_list->itemWidget(item);
        if (!widget) {
            return;
        }

        auto con_item = qobject_cast<ChatUserWid*>(widget);
        if (!con_item) {
            return;
        }

        //设置信息
        auto chat_data = con_item->GetChatData();
        ui->chat_page->SetChatData(chat_data);
        return;
    }

    auto find_iter = _chat_thread_items.find(thread_id);
    if (find_iter == _chat_thread_items.end()) {
        return;
    }

    //转为widget
    QWidget* widget = ui->chat_user_list->itemWidget(find_iter.value());
    if (!widget) {
        return;
    }

    //判断转化为自定义的widget
    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if (itemType == CHAT_USER_ITEM) {
        auto con_item = qobject_cast<ChatUserWid*>(customItem);
        if (!con_item) {
            return;
        }

        //设置信息
        auto chat_data = con_item->GetChatData();
        ui->chat_page->SetChatData(chat_data);

        return;
    }

}

void ChatDialog::slot_loading_contact_user()
{
    qDebug() << "slot loading contact user";
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    loadMoreConUser();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;
}

void ChatDialog::slot_friend_info_page(std::shared_ptr<UserInfo> user_info)
{
    qDebug()<<"receive switch friend info page sig";
    _last_widget = ui->friend_info_page;
    ui->stackedWidget->setCurrentWidget(ui->friend_info_page);
    ui->friend_info_page->SetInfo(user_info);
}
void ChatDialog::showSearch(bool b_search)
{
    if(b_search){
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        _mode = ChatUIMode::SearchMode;
    }else if(_state == ChatUIMode::ChatMode){
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        _mode = ChatUIMode::ChatMode;
        ui->search_list->CloseFindDlg();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }else if(_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
        ui->search_list->CloseFindDlg();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }else if(_state == ChatUIMode::SettingsMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
        ui->search_list->CloseFindDlg();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }
}

void ChatDialog::AddLBGroup(StateWidget *lb)
{
    _lb_list.push_back(lb);
}

void ChatDialog::loadMoreConUser()
{
    auto friend_list = UserMgr::GetInstance()->GetConListPerPage();
    if (friend_list.empty() == false) {
        for(auto & friend_ele : friend_list){
            auto *chat_user_wid = new ConUserItem();
            chat_user_wid->SetInfo(friend_ele->_uid,friend_ele->_name,
                                   friend_ele->_icon);
            QListWidgetItem *item = new QListWidgetItem;
            //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
            item->setSizeHint(chat_user_wid->sizeHint());
            ui->con_user_list->addItem(item);
            ui->con_user_list->setItemWidget(item, chat_user_wid);
        }

        //更新已加载条目
        UserMgr::GetInstance()->UpdateContactLoadedCount();
    }
}

void ChatDialog::UpdateChatMsg(std::vector<std::shared_ptr<TextChatData> > msgdata)
{
    for (auto& msg : msgdata) {
        if (msg->GetThreadId() != _cur_chat_thread_id) {
            break;
        }

        ui->chat_page->AppendChatMsg(msg);
    }
}


void ChatDialog::LoadHeadIcon(QString avatarPath, QLabel* icon_label, QString file_name,QString req_type) {
//    UserMgr::GetInstance()->AddLabelToReset(avatarPath, icon_label);
//    //先加载默认的
//    QPixmap pixmap(":/res/head_1.jpg");
//    QPixmap scaledPixmap = pixmap.scaled(icon_label->size(),
//        Qt::KeepAspectRatio, Qt::SmoothTransformation); // 将图片缩放到label的大小
//    icon_label->setPixmap(scaledPixmap); // 将缩放后的图片设置到QLabel上
//    icon_label->setScaledContents(true); // 设置QLabel自动缩放图片内容以适应大小

//    //判断是否正在下载
//    bool is_loading = UserMgr::GetInstance()->IsDownLoading(file_name);
//    if (is_loading) {
//        qWarning() << "正在下载: " << file_name;
//    }
//    else {
//        //发送请求获取资源
//        auto download_info = std::make_shared<DownloadInfo>();
//        download_info->_name = file_name;
//        download_info->_current_size = 0;
//        download_info->_seq = 1;
//        download_info->_total_size = 0;
//        download_info->_client_path = avatarPath;
//        //添加文件到管理者
//        UserMgr::GetInstance()->AddDownloadFile(file_name, download_info);
//        //发送消息
//        FileTcpMgr::GetInstance()->SendDownloadInfo(download_info, req_type);
//    }
}
void ChatDialog::slot_loading_chat_user()
{
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    //loadMoreChatUser();

    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;
}

void ChatDialog::slot_side_chat()
{
    qDebug()<< "receive side chat clicked";
    ClearLabelState(ui->side_chat_lb);
    ui->stackedWidget->setCurrentWidget(ui->chat_page);
    _state = ChatUIMode::ChatMode;

    ui->search_edit->clear();
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));//空白图片
    ui->search_edit->clearFocus();
    showSearch(false);
}

void ChatDialog::slot_side_contact()
{
    qDebug()<< "receive side contact clicked";
    ClearLabelState(ui->side_contact_lb);
    //设置
//    if(_last_widget == nullptr){
//        ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
//        _last_widget = ui->friend_apply_page;
//    }else{
//        ui->stackedWidget->setCurrentWidget(_last_widget);
//    }
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
    _state = ChatUIMode::ContactMode;

    ui->search_edit->clear();
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));//空白图片
    ui->search_edit->clearFocus();
    showSearch(false);
}

void ChatDialog::slot_text_changed(const QString &str)
{
    //qDebug()<< "receive slot text changed str is " << str;
    if (!str.isEmpty()) {
        showSearch(true);
    }else{
        showSearch(false);
    }
}

void ChatDialog::slot_apply_friend(std::shared_ptr<AddFriendApply> apply)
{
    qDebug()<< "receive slot_apply_friend  applyuid is  "<<apply->_from_uid<< " name is  "<<apply->_name<<"  desc is "<<apply->_desc;
    bool b_already=UserMgr::GetInstance()->AlreadyApply(apply->_from_uid);
    if(b_already){
        return;
    }
    UserMgr::GetInstance()->AddApplyList(std::make_shared<ApplyInfo>(apply));
    ui->side_contact_lb->ShowRedPoint(true);
    ui->con_user_list->ShowRedPoint(true);
    ui->friend_apply_page->AddNewApply(apply);


}
void ChatDialog::slot_jump_chat_item(std::shared_ptr<SearchInfo> si)
{
    qDebug() << "slot jump chat item " << endl;
    auto chat_thread_data = UserMgr::GetInstance()->GetChatThreadByUid(si->_uid);
    if (chat_thread_data) {
        auto find_iter = _chat_thread_items.find(chat_thread_data->GetThreadId());
        if (find_iter != _chat_thread_items.end()) {
            qDebug() << "jump to chat item , uid is " << si->_uid;
            ui->chat_user_list->scrollToItem(find_iter.value());
            ui->side_chat_lb->SetSelected(true);
            SetSelectChatItem(chat_thread_data->GetThreadId());
            //更新聊天界面信息
            SetSelectChatPage(chat_thread_data->GetThreadId());
            slot_side_chat();
            return;
        } //说明之前有缓存过聊天列表，只是被删除了，那么重新加进来即可
        else {
            auto* chat_user_wid = new ChatUserWid();
            chat_user_wid->SetChatData(chat_thread_data);
            QListWidgetItem* item = new QListWidgetItem;
            qDebug() << "chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
            ui->chat_user_list->insertItem(0, item);
            ui->chat_user_list->setItemWidget(item, chat_user_wid);
            _chat_thread_items.insert(chat_thread_data->GetThreadId(), item);
            ui->side_chat_lb->SetSelected(true);
            SetSelectChatItem(chat_thread_data->GetThreadId());
            //更新聊天界面信息
            SetSelectChatPage(chat_thread_data->GetThreadId());
            slot_side_chat();
            return;
        }
    }

    //如果没找到，则发送创建请求
    auto uid = UserMgr::GetInstance()->GetUid();
    QJsonObject jsonObj;
    jsonObj["uid"] = uid;
    jsonObj["other_id"] = si->_uid;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //发送tcp请求给chat server
    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CREATE_PRIVATE_CHAT_REQ, jsonData);

}

void ChatDialog::slot_side_setting(){
    qDebug()<< "receive side setting clicked";
    ClearLabelState(ui->side_settings_lb);
    //设置
    ui->stackedWidget->setCurrentWidget(ui->user_info_page);

    _state = ChatUIMode::SettingsMode;
    showSearch(false);
}
void ChatDialog::slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info) {
    qDebug() << "receive slot_add_auth__friend uid is " << auth_info->_uid
        << " name is " << auth_info->_name << " nick is " << auth_info->_nick;

    //判断如果已经是好友则跳过
    auto bfriend = UserMgr::GetInstance()->CheckFriendById(auth_info->_uid);
    if (bfriend) {
        return;
    }

    UserMgr::GetInstance()->AddFriend(auth_info);

    auto chat_thread_data = std::make_shared<ChatThreadData>(auth_info->_uid, auth_info->_thread_id, 0);
    UserMgr::GetInstance()->AddChatThreadData(chat_thread_data, auth_info->_uid);
    for (auto& chat_msg : auth_info->_chat_datas) {
        chat_thread_data->AppendMsg(chat_msg->GetMsgId(), chat_msg);
    }

    auto iter = _chat_thread_items.find(auth_info->_thread_id);
    if (iter != _chat_thread_items.end()) {
        return;
    }

    auto* chat_user_wid = new ChatUserWid();
    chat_user_wid->SetChatData(chat_thread_data);
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_thread_items.insert(auth_info->_thread_id, item);
}

void ChatDialog::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    qDebug() << "receive slot_auth_rsp uid is " << auth_rsp->_uid
        << " name is " << auth_rsp->_name << " nick is " << auth_rsp->_nick;

    //判断如果已经是好友则跳过
    auto bfriend = UserMgr::GetInstance()->CheckFriendById(auth_rsp->_uid);
    if (bfriend) {
        return;
    }

    UserMgr::GetInstance()->AddFriend(auth_rsp);
    auto* chat_user_wid = new ChatUserWid();
    auto chat_thread_data = std::make_shared<ChatThreadData>(auth_rsp->_uid, auth_rsp->_thread_id, 0);
    UserMgr::GetInstance()->AddChatThreadData(chat_thread_data, auth_rsp->_uid);
    for (auto& chat_msg : auth_rsp->_chat_datas) {
        chat_thread_data->AppendMsg(chat_msg->GetMsgId(), chat_msg);
    }
    chat_user_wid->SetChatData(chat_thread_data);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_thread_items.insert(auth_rsp->_thread_id, item);
}

void ChatDialog::slot_switch_apply_friend_page()
{
    qDebug()<<"receive switch apply friend page sig";
    _last_widget = ui->friend_apply_page;
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
}

void ChatDialog::slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info)
{
    qDebug() << "slot jump chat item " << endl;
    //看是否已经存在聊天会话
    auto chat_thread_data = UserMgr::GetInstance()->GetChatThreadByUid(user_info->_uid);
    if (chat_thread_data) {
        auto find_iter = _chat_thread_items.find(chat_thread_data->GetThreadId());
        //如果侧边栏找到了，说明之前有缓存过聊天列表，直接跳转已有聊天
        if (find_iter != _chat_thread_items.end()) {
            qDebug() << "jump to chat item , uid is " << user_info->_uid;
            ui->chat_user_list->scrollToItem(find_iter.value());//滚动到那个聊天条目
            ui->side_chat_lb->SetSelected(true);
            SetSelectChatItem(chat_thread_data->GetThreadId());
            //更新聊天界面信息
            SetSelectChatPage(chat_thread_data->GetThreadId());
            slot_side_chat();
            return;
        }
        //有 thread 数据，但 UI 上被删除过 再次添加即可
        else {
            auto* chat_user_wid = new ChatUserWid();
            chat_user_wid->SetChatData(chat_thread_data);
            QListWidgetItem* item = new QListWidgetItem;
            qDebug() << "chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
            ui->chat_user_list->insertItem(0, item);
            ui->chat_user_list->setItemWidget(item, chat_user_wid);
            _chat_thread_items.insert(chat_thread_data->GetThreadId(), item);
            ui->side_chat_lb->SetSelected(true);
            SetSelectChatItem(chat_thread_data->GetThreadId());
            //更新聊天界面信息
            SetSelectChatPage(chat_thread_data->GetThreadId());
            slot_side_chat();
            return;
        }
    }

    //如果没找到，也就是没创建聊天私聊thread，则发送创建请求
    auto uid = UserMgr::GetInstance()->GetUid();
    QJsonObject jsonObj;
    jsonObj["uid"] = uid;
    jsonObj["other_id"] = user_info->_uid;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //发送tcp请求给chat server
    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CREATE_PRIVATE_CHAT_REQ, jsonData);
}


void ChatDialog::slot_item_clicked(QListWidgetItem* item)
{
    QWidget* widget = ui->chat_user_list->itemWidget(item); // 获取自定义widget对象
    if (!widget) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if (itemType == ListItemType::INVALID_ITEM
        || itemType == ListItemType::GROUP_TIP_ITEM) {
        qDebug() << "slot invalid item clicked ";
        return;
    }


    if (itemType == ListItemType::CHAT_USER_ITEM) {
        // 创建对话框，提示用户
        qDebug() << "contact user item clicked ";

        auto chat_wid = qobject_cast<ChatUserWid*>(customItem);
        auto chat_data = chat_wid->GetChatData();
        //跳转到聊天界面
        ui->chat_page->SetChatData(chat_data);
        _cur_chat_thread_id = chat_data->GetThreadId();
        return;
    }
}

//添加聊天消息, 将消息放到用户区和thread_id关联
void ChatDialog::slot_text_chat_msg(std::vector<std::shared_ptr<TextChatData>> msglists)
{
    for (auto& msg : msglists) {

        //更新数据
        auto thread_id = msg->GetThreadId();
        auto thread_data = UserMgr::GetInstance()->GetChatThreadByThreadId(thread_id);

        thread_data->AddMsg(msg);

        if (_cur_chat_thread_id != thread_id) {
            continue;
        }

        ui->chat_page->AppendChatMsg(msg);
    }

}

//加载会话列表，加载完后加载聊天信息
void ChatDialog::slot_load_chat_thread(bool load_more, int last_thread_id,
    std::vector<std::shared_ptr<ChatThreadInfo>> chat_threads)
{
    for (auto& cti : chat_threads) {
        //先处理单聊，群聊跳过，以后添加
        if (cti->_type == "group") {
            continue;
        }

        auto uid = UserMgr::GetInstance()->GetUid();
        auto other_uid = 0;
        if (uid == cti->_user1_id) {
            other_uid = cti->_user2_id;
        }
        else {
            other_uid = cti->_user1_id;
        }

        auto chat_thread_data = std::make_shared<ChatThreadData>(other_uid, cti->_thread_id, 0);
        //建立会话id到聊天数据数据的映射关系，存储会话列表，将对方uid和会话id关联
        UserMgr::GetInstance()->AddChatThreadData(chat_thread_data, other_uid);

        auto* chat_user_wid = new ChatUserWid();
        chat_user_wid->SetChatData(chat_thread_data);
        QListWidgetItem* item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(chat_user_wid->sizeHint());
        ui->chat_user_list->addItem(item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
        _chat_thread_items.insert(cti->_thread_id, item);
        //如果需要访问 QWidget *widget = ui->chat_user_list->itemWidget(item);
    }

    UserMgr::GetInstance()->SetLastChatThreadId(last_thread_id);

    if (load_more) {
        //发送请求逻辑
        QJsonObject jsonObj;
        auto uid = UserMgr::GetInstance()->GetUid();
        jsonObj["uid"] = uid;
        jsonObj["thread_id"] = last_thread_id;


        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        //发送tcp请求给chat server
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_THREAD_REQ, jsonData);
        return;
    }

    showLoadingDlg(false);
    //继续加载聊天数据
    loadChatMsg();
}

void ChatDialog::slot_create_private_chat(int uid, int other_id, int thread_id)
{
    auto* chat_user_wid = new ChatUserWid();
    auto chat_thread_data = std::make_shared<ChatThreadData>(other_id, thread_id, 0);
    if (chat_thread_data == nullptr) {
        return;
    }
    UserMgr::GetInstance()->AddChatThreadData(chat_thread_data, other_id);

    chat_user_wid->SetChatData(chat_thread_data);
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    qDebug() << "chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_thread_items.insert(thread_id, item);

    ui->side_chat_lb->SetSelected(true);
    SetSelectChatItem(thread_id);
    //更新聊天界面信息
    SetSelectChatPage(thread_id);
    slot_side_chat();
    return;
}



void ChatDialog::slot_load_chat_msg(int thread_id, int msg_id, bool load_more, std::vector<std::shared_ptr<TextChatData>> msglists)
{
    _cur_load_chat->SetLastMsgId(msg_id);//记录最后一个msg_id
    //加载聊天信息
    for (auto& chat_msg : msglists) {
        _cur_load_chat->AppendMsg(chat_msg->GetMsgId(), chat_msg);
    }

    //还有未加载完的消息，就继续加载
    if (load_more) {
        //发送请求给服务器
            //发送请求逻辑
        QJsonObject jsonObj;
        jsonObj["thread_id"] = _cur_load_chat->GetThreadId();
        jsonObj["message_id"] = _cur_load_chat->GetLastMsgId();

        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        //发送tcp请求给chat server
        emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_MSG_REQ, jsonData);
        return;
    }

    //获取下一个chat_thread
    _cur_load_chat = UserMgr::GetInstance()->GetNextLoadData();
    //都加载完了
    if(!_cur_load_chat){
        //更新聊天界面信息
        SetSelectChatItem();
        SetSelectChatPage();
        showLoadingDlg(false);
        return;
    }

    //继续加载下一个聊天
    //发送请求给服务器
    //发送请求逻辑
    QJsonObject jsonObj;
    jsonObj["thread_id"] = _cur_load_chat->GetThreadId();
    jsonObj["message_id"] = _cur_load_chat->GetLastMsgId();

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //发送tcp请求给chat server
    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_LOAD_CHAT_MSG_REQ, jsonData);
}

void ChatDialog::slot_add_chat_msg(int thread_id, std::vector<std::shared_ptr<TextChatData>> msglists) {
    auto chat_data = UserMgr::GetInstance()->GetChatThreadByThreadId(thread_id);
    if (chat_data == nullptr) {
        return;
    }

    //将消息放入数据中管理
    for (auto& msg : msglists) {
        chat_data->MoveMsg(msg);

        if (_cur_chat_thread_id != thread_id) {
            continue;
        }
        //更新聊天界面信息
        ui->chat_page->UpdateChatStatus(msg);
    }
}
