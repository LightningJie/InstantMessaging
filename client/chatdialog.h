#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include  "global.h"
#include "statewidget.h"
#include "userdata.h"

#include <QListWidgetItem>
namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
protected:
    //类内部可以用 子类可以重写/调用 ChatDialog::eventFilter(...)外部不能乱调
    //Qt 不是“某个外部对象拿着 ChatDialog* 去写 dlg->eventFilter(...) 这种普通函数调用”。
    //它是通过 QObject 的虚函数机制 在 QObject 内部调用的——调用发生在基类内部，所以访问权限不受你上述的那种“外部调用限制”。
    bool eventFilter(QObject *watched, QEvent *event) override ;
    void handleGlobalMousePress(QMouseEvent *event);

private:
    void addChatUserList();
    void ClearLabelState(StateWidget *lb);
    void showSearch(bool b_search=false);
    void AddLBGroup(StateWidget *lb);
    void loadMoreChatUser();
    void loadMoreConUser();
    void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata);
    Ui::ChatDialog *ui;
    ChatUIMode _mode;
    ChatUIMode  _state;
    QList<StateWidget *> _lb_list;
    bool _b_loading;
    QAction *searchAction;
    QAction *clearAction;
    QMap<int,QListWidgetItem *> _chat_items_added;
    int _cur_chat_uid;
    QWidget* _last_widget;
private slots:
    void slot_loading_chat_user();
    void slot_side_chat();
    void slot_side_contact();
    void slot_text_changed(const QString &str);
    //void slot_side_setting();
    void slot_add_auth_friend(std::shared_ptr<AuthInfo>);
    void slot_auth_rsp(std::shared_ptr<AuthRsp>);
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
    void slot_jump_chat_item(std::shared_ptr<SearchInfo>);
    void SetSelectChatItem(int uid = 0);
    void SetSelectChatPage(int uid = 0);
    void slot_loading_contact_user();
    void slot_friend_info_page(std::shared_ptr<UserInfo> user_info);
    void slot_switch_apply_friend_page();
    void slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info);
    void slot_item_clicked(QListWidgetItem *item);
    void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
    void slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata);
};

#endif // CHATDIALOG_H
