#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include  "global.h"
#include "statewidget.h"
#include "userdata.h"
#include <QTimer>
#include <QListWidgetItem>
#include "loadingdlg.h"
namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    void loadChatList();
    void loadChatMsg();
    ~ChatDialog();
protected:
    //类内部可以用 子类可以重写/调用 ChatDialog::eventFilter(...)外部不能乱调
    //Qt 不是“某个外部对象拿着 ChatDialog* 去写 dlg->eventFilter(...) 这种普通函数调用”。
    //它是通过 QObject 的虚函数机制 在 QObject 内部调用的——调用发生在基类内部，所以访问权限不受你上述的那种“外部调用限制”。
    bool eventFilter(QObject *watched, QEvent *event) override ;
    void handleGlobalMousePress(QMouseEvent *event);

private:
    void showLoadingDlg(bool show = true);
    void AddLBGroup(StateWidget* lb);
    void ClearLabelState(StateWidget* lb);
    void loadMoreConUser();
    void SetSelectChatItem(int thread_id = 0);
    void SetSelectChatPage(int thread_id = 0);
    void showSearch(bool b_search=false);
    void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata);
    void CloseFindDlg();
    void LoadHeadIcon(QString avatarPath, QLabel* icon_label, QString file_name, QString req_type);
    Ui::ChatDialog *ui;
    ChatUIMode _mode;
    ChatUIMode  _state;
    QList<StateWidget *> _lb_list;
    bool _b_loading;
    QAction *searchAction;
    QAction *clearAction;
    int _cur_chat_uid;
    QWidget* _last_widget;
    QTimer  *_timer;
    //chat_thread_id 对应的item对应关系
    QMap<int, QListWidgetItem*>  _chat_thread_items;
    int _cur_chat_thread_id;
    LoadingDlg* _loading_dlg;
    std::shared_ptr<ChatThreadData> _cur_load_chat;

private slots:
    void slot_loading_chat_user();
    void slot_side_chat();
    void slot_side_contact();
    void slot_text_changed(const QString &str);
    void slot_side_setting();
    void slot_add_auth_friend(std::shared_ptr<AuthInfo>);
    void slot_auth_rsp(std::shared_ptr<AuthRsp>);
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
    void slot_jump_chat_item(std::shared_ptr<SearchInfo>);
    void slot_loading_contact_user();
    void slot_friend_info_page(std::shared_ptr<UserInfo> user_info);
    void slot_switch_apply_friend_page();
    void slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info);
    void slot_item_clicked(QListWidgetItem *item);
    void slot_text_chat_msg(std::vector<std::shared_ptr<TextChatData>> msglists);
    void slot_load_chat_thread(bool load_more, int last_thread_id,
    std::vector<std::shared_ptr<ChatThreadInfo>> chat_threads);

    void slot_create_private_chat(int uid, int other_id, int thread_id);

    void slot_load_chat_msg(int thread_id, int msg_id, bool load_more,
    std::vector<std::shared_ptr<TextChatData>> msglists);

    void slot_add_chat_msg(int thread_id, std::vector<std::shared_ptr<TextChatData>> msglists);
//    void slot_add_img_msg(int thread_id, std::shared_ptr<ImgChatData> img_msg);
//    void slot_reset_icon(QString path);
//    void slot_update_upload_progress(std::shared_ptr<MsgInfo> msg_info);
//    void slot_update_download_progress(std::shared_ptr<MsgInfo> msg_info);
//    void slot_download_finish(std::shared_ptr<MsgInfo> msg_info, QString file_path);:
//    void slot_reset_head();
};

#endif // CHATDIALOG_H
