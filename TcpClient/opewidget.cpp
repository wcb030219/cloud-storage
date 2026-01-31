#include "opewidget.h"
#include <QVBoxLayout>

opewidget::opewidget(QWidget *parent)
    : QWidget{parent}
{
    m_pUserLabel = new QLabel("当前用户：未登录", this);
    m_pUserLabel->setStyleSheet("font-weight: bold; color: blue; font-size: 14px;");
    m_pUserLabel->setAlignment(Qt::AlignLeft);
    setFixedSize(860,550);
    m_pListW = new QListWidget(this);
    m_pListW->addItem("好友");
    m_pListW->addItem("图书");

    m_pFriend = new Friend;
    m_pBook = new Book;

    m_pSW = new QStackedWidget;
    m_pSW->addWidget(m_pFriend);
    m_pSW->addWidget(m_pBook);

    QVBoxLayout *pMainVBL = new QVBoxLayout;
    pMainVBL->addWidget(m_pUserLabel);
    pMainVBL->addSpacing(5);

    QHBoxLayout *pMain = new QHBoxLayout;
    pMain->addWidget(m_pListW);
    pMain->addWidget(m_pSW);

    pMainVBL->addLayout(pMain);

    setLayout(pMainVBL);

    connect(m_pListW,&QListWidget::currentRowChanged,m_pSW,&QStackedWidget::setCurrentIndex);
}

void opewidget::setLoginName(const QString &name)
{
    m_pUserLabel->setText(QString("当前用户：%1").arg(name));
}

opewidget &opewidget::getInstance()
{
    static opewidget instance;
    return instance;
}

Friend *opewidget::getFriend()
{
    return m_pFriend;
}

Book *opewidget::getBook()
{
    return m_pBook;
}
