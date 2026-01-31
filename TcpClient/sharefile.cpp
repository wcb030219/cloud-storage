#include "sharefile.h"
#include "tcpclient.h"
#include "opewidget.h"
#include "protocol.h"
#include <QDebug>
#include <QMessageBox>
#include <QByteArray>

ShareFile::ShareFile(QWidget *parent)
    : QWidget{parent}
{
    m_pSelectAllPB = new QPushButton("全选");
    m_pCancelSelectPB = new QPushButton("取消选择");
    m_pOKPB = new QPushButton("确定");
    m_pCancelPB = new QPushButton("取消");

    m_pSA = new QScrollArea;
    m_pFriendW = new QWidget;
    m_pFriendWVBL = new QVBoxLayout(m_pFriendW);
    m_pButtonGroup = new QButtonGroup(m_pFriendW);
    m_pButtonGroup->setExclusive(false);
    m_pSA->setWidgetResizable(true);

    QHBoxLayout *pTopHBL  = new QHBoxLayout;
    pTopHBL->addWidget(m_pSelectAllPB);
    pTopHBL->addWidget(m_pCancelSelectPB);
    pTopHBL->addStretch();

    QHBoxLayout *pDownHBL  = new QHBoxLayout;
    pDownHBL->addWidget(m_pOKPB);
    pDownHBL->addWidget(m_pCancelPB);

    QVBoxLayout *pMainVBL = new QVBoxLayout;
    pMainVBL->addLayout(pTopHBL);
    pMainVBL->addWidget(m_pSA);
    pMainVBL->addLayout(pDownHBL);

    setLayout(pMainVBL);

    resize(400, 300);
    setWindowTitle("分享文件");

    connect(m_pCancelSelectPB, &QPushButton::clicked, this, &ShareFile::cancelSelect);
    connect(m_pSelectAllPB, &QPushButton::clicked, this, &ShareFile::selectAll);
    connect(m_pOKPB, &QPushButton::clicked, this, &ShareFile::okShare);
    connect(m_pCancelPB, &QPushButton::clicked, this, &ShareFile::cancelShare);
    m_pSA->setWidget(m_pFriendW);


}

ShareFile &ShareFile::getInstance()
{
    static ShareFile instance;
    return instance;
}

void ShareFile::updateFriend(QListWidget *pFriendList)
{
    if(nullptr == pFriendList) return;

    // ✅ 先清空按钮（delete widget 会自动从布局移除）
    while(!m_pButtonGroup->buttons().isEmpty()) {
        QAbstractButton *btn = m_pButtonGroup->buttons().first();
        m_pButtonGroup->removeButton(btn);
        delete btn;  // 这里已经从布局移除了
    }

    // ✅ 清理剩余的 stretch items（此时已经没有 widget 了）
    while(m_pFriendWVBL->count() > 0) {
        QLayoutItem *item = m_pFriendWVBL->takeAt(0);
        delete item;  // 安全，只删除 layout item
    }

    // 添加新复选框
    for(int i = 0; i < pFriendList->count(); i++) {
        QListWidgetItem *pItem = pFriendList->item(i);
        // 设置父对象 m_pFriendW，确保生命周期管理
        QCheckBox *pCB = new QCheckBox(pItem->text(), m_pFriendW);
        m_pFriendWVBL->addWidget(pCB);
        m_pButtonGroup->addButton(pCB);
    }
    m_pFriendWVBL->addStretch();
}

void ShareFile::setFileInfo(const QString &strPath, const QString &strFileName)
{
    m_strFilePath = strPath;
    m_strFileName = strFileName;
}


void ShareFile::selectAll()
{
    for(QAbstractButton *btn : m_pButtonGroup->buttons()) {
        QCheckBox *cb = dynamic_cast<QCheckBox*>(btn);
        if(cb) cb->setChecked(true);
    }
}

void ShareFile::cancelSelect()
{
    for(QAbstractButton *btn : m_pButtonGroup->buttons()) {
        QCheckBox *cb = dynamic_cast<QCheckBox*>(btn);
        if(cb) cb->setChecked(false);
    }
}

void ShareFile::okShare()
{
    // 收集选中的好友
    QStringList strReceiverList;
    for(QAbstractButton *btn : m_pButtonGroup->buttons()) {
        QCheckBox *cb = dynamic_cast<QCheckBox*>(btn);
        if(cb && cb->isChecked()) {
            strReceiverList.append(cb->text().trimmed());
        }
    }

    if(strReceiverList.isEmpty()) {
        QMessageBox::warning(this, "分享文件", "请至少选择一个好友");
        return;
    }

    if (m_strFilePath.isEmpty() || m_strFileName.isEmpty()) {
        QMessageBox::warning(this, "分享文件", "文件信息不完整");
        return;
    }

    // 构造消息：接收者1,接收者2,...|当前路径|文件名
    // 使用特殊分隔符避免与路径中的/冲突
    QString strReceivers = strReceiverList.join(",");
    QString strMsg = QString("%1|%2|%3")
                         .arg(strReceivers)
                         .arg(m_strFilePath)
                         .arg(m_strFileName);

    QByteArray msgBytes = strMsg.toUtf8();

    PDU *pdu = mkPDU(msgBytes.size() + 1);
    if (!pdu) {
        QMessageBox::warning(this, "分享文件", "内存分配失败");
        return;
    }

    pdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_REQUEST;  // 注意：这里是REQUEST不是RESPOND

    // 填充发送者名字到caData前32字节
    QString strSender = TcpClient::getInstance().loginName();
    QByteArray senderBytes = strSender.toUtf8();
    memset(pdu->caData, 0, 64);
    int copyLen = qMin(senderBytes.size(), 31);
    if (copyLen > 0) {
        memcpy(pdu->caData, senderBytes.constData(), copyLen);
    }

    // 填充消息体
    memcpy(pdu->caMsg, msgBytes.constData(), msgBytes.size());
    pdu->caMsg[msgBytes.size()] = '\0';
    pdu->uiMsgLen = msgBytes.size() + 1;

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);

    QMessageBox::information(this, "分享文件", "分享请求已发送");
    hide();
}

void ShareFile::cancelShare()
{
    hide();
}
