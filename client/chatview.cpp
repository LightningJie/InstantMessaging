#include "chatview.h"
#include <QScrollBar>
#include <QVBoxLayout>
#include <QEvent>
#include <QDebug>

#include <QTimer>
#include <QStyleOption>
#include <QPainter>
#include "chatitembase.h"
ChatView::ChatView(QWidget *parent):QWidget(parent),isAppended(false)
{
    QVBoxLayout *pMainLayout=new QVBoxLayout();
    this->setLayout(pMainLayout);
    pMainLayout->setMargin(0);

    m_pScrollArea = new QScrollArea();//创建滚动区域
    m_pScrollArea->setObjectName("chat_area");
    pMainLayout->addWidget(m_pScrollArea);

    QWidget *w = new QWidget(this);
    w->setObjectName("chat_bg");
    w->setAutoFillBackground(true);//QWidget 的背景请你帮我自动刷出来，不要透明露出下面的东西
    QVBoxLayout *pHLayout_1 = new QVBoxLayout();
    pHLayout_1->addWidget(new QWidget(), 100000);//stretch 表示伸展系数  只要布局里有多余空间，就几乎都分给它
    w->setLayout(pHLayout_1);
    m_pScrollArea->setWidget(w);    //应该时在QSCrollArea构造后执行 才对

    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//永不显示
    QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
    connect(pVScrollBar, &QScrollBar::rangeChanged,this, &ChatView::onVScrollBarMoved);//内容高度/可视区域高度变了，导致“能滚动的最大值”变了 自动滚到最底部
    //把垂直ScrollBar放到上边 而不是原来的并排
    QHBoxLayout *pHLayout_2 = new QHBoxLayout();
    pHLayout_2->addWidget(pVScrollBar, 0, Qt::AlignRight);
    pHLayout_2->setMargin(0);
    m_pScrollArea->setLayout(pHLayout_2);
    pVScrollBar->setHidden(true);

    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->installEventFilter(this);
    initStyleSheet();
}

void ChatView::appendChatItem(QWidget *item)//插到“倒数第一个 item”之前
{
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    qDebug() << "vl->count() is " << vl->count();
    vl->insertWidget(vl->count()-1, item);
    isAppended = true;
}

void ChatView::prependChatItem(QWidget *item)
{

}

void ChatView::insertChatItem(QWidget *before, QWidget *item)
{

}

void ChatView::removeAllItem()
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());

   int count = layout->count();

    for (int i = 0; i < count - 1; ++i) {
        QLayoutItem *item = layout->takeAt(0); // 始终从第一个控件开始删除
        if (item) {
            if (QWidget *widget = item->widget()) {
                delete widget;
            }
            delete item;
        }
    }
}

bool ChatView::eventFilter(QObject *o, QEvent *e)
{
    /*if(e->type() == QEvent::Resize && o == )
    {

    }
    else */if(e->type() == QEvent::Enter && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(m_pScrollArea->verticalScrollBar()->maximum() == 0);
    }
    else if(e->type() == QEvent::Leave && o == m_pScrollArea)
    {
         m_pScrollArea->verticalScrollBar()->setHidden(true);
    }
    return QWidget::eventFilter(o, e);
}
void ChatView::onVScrollBarMoved(int min, int max)
{
    if(isAppended) //添加item可能调用多次
    {
        QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
        pVScrollBar->setSliderPosition(pVScrollBar->maximum());
        //500毫秒内可能调用多次
        QTimer::singleShot(500, [this]()//异步（延迟）执行
        {
            isAppended = false;
        });
    }
}
void ChatView::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
void ChatView::initStyleSheet()
{
//    QScrollBar *scrollBar = m_pScrollArea->verticalScrollBar();
//    scrollBar->setStyleSheet("QScrollBar{background:transparent;}"
//                             "QScrollBar:vertical{background:transparent;width:8px;}"
//                             "QScrollBar::handle:vertical{background:red; border-radius:4px;min-height:20px;}"
//                             "QScrollBar::add-line:vertical{height:0px}"
//                             "QScrollBar::sub-line:vertical{height:0px}"
//                             "QScrollBar::add-page:vertical {background:transparent;}"
//                             "QScrollBar::sub-page:vertical {background:transparent;}");
}
