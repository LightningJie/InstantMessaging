#include "chatitembase.h"



ChatItemBase::ChatItemBase(ChatRole role, QWidget *parent):QWidget(parent),m_role(role)
{
    //创建namelabel  设置label字体
    m_pNameLabel =new QLabel(this);
    m_pNameLabel->setObjectName("chat_user_name");
    QFont font("Microsoft YaHe");
    font.setPointSize(9);
    m_pNameLabel->setFont(font);
    m_pNameLabel->setFixedHeight(20);//固定高度20

    //创建iconlabel
    m_pIconLabel =new QLabel(this);
    m_pIconLabel->setObjectName("chat_user_icon");
    m_pIconLabel->setScaledContents(true);//把图片自动缩放到 QLabel 当前的尺寸
    m_pIconLabel->setFixedSize(42,42);

    //创建气泡框
    m_pBubble =new QWidget(this);

    //创建网格布局
    QGridLayout *pGLayout = new QGridLayout();

    pGLayout->setVerticalSpacing(3);
    pGLayout->setHorizontalSpacing(3);
    pGLayout->setMargin(3);

    //创建弹簧
    QSpacerItem*pSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);//高40*宽20
    if(m_role == ChatRole::Self)
    {
        m_pNameLabel->setContentsMargins(0,0,8,0);//左上右下
        m_pNameLabel->setAlignment(Qt::AlignRight);
        pGLayout->addWidget(m_pNameLabel, 0,1, 1,1); //0行1列占1行1列
        pGLayout->addWidget(m_pIconLabel, 0, 2, 2,1, Qt::AlignTop);
        pGLayout->addItem(pSpacer, 1, 0, 1, 1);
        pGLayout->addWidget(m_pBubble, 1,1, 1,1);
        pGLayout->setColumnStretch(0, 2);//第0列40%
        pGLayout->setColumnStretch(1, 3);//拉伸时候 第一列百分之60
    }else{
        m_pNameLabel->setContentsMargins(8,0,0,0);
        m_pNameLabel->setAlignment(Qt::AlignLeft);
        pGLayout->addWidget(m_pIconLabel, 0, 0, 2,1, Qt::AlignTop);
        pGLayout->addWidget(m_pNameLabel, 0,1, 1,1);
        pGLayout->addWidget(m_pBubble, 1,1, 1,1);
        pGLayout->addItem(pSpacer, 2, 2, 1, 1);
        pGLayout->setColumnStretch(1, 3);
        pGLayout->setColumnStretch(2, 2);
    }
    this->setLayout(pGLayout);
}
void ChatItemBase::setUserName(const QString &name)
{
    m_pNameLabel->setText(name);
}

void ChatItemBase::setUserIcon(const QPixmap &icon)
{
    m_pIconLabel->setPixmap(icon);
}

void ChatItemBase::setWidget(QWidget *w)
{
   QGridLayout *pGLayout = (qobject_cast<QGridLayout *>)(this->layout());
   pGLayout->replaceWidget(m_pBubble, w);
   delete m_pBubble;
   m_pBubble = w;
}
