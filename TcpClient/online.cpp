#include "online.h"
#include "ui_online.h"
#include "tcpclient.h"

Online::Online(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Online)
{
    ui->setupUi(this);
}

Online::~Online()
{
    delete ui;
}

void Online::showUsr(PDU *pdu)
{
    if(NULL == pdu){
        return;
    }
    ui->online_lw->clear();
    uint uiSize = pdu->uiMsgLen/32;
    char caTmp[32];
    QString strMyName = TcpClient::getInstance().loginName();
    for(int i =0;i<uiSize;i++){
        memcpy(caTmp,pdu->caMsg+i*32,32);
        QString strUserName = QString::fromUtf8(caTmp).trimmed();
        if(strUserName == strMyName){
            continue;
        }
        ui->online_lw->addItem(caTmp);
    }
}

void Online::on_addfriend_pb_clicked()
{
    QListWidgetItem *pItem =  ui->online_lw->currentItem();
    if(nullptr == pItem){
        return;
    }
    QString strPerUsrName =  pItem->text();
    QString strLoginName = TcpClient::getInstance().loginName();
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
    memcpy(pdu->caData,strPerUsrName.toStdString().c_str(),strPerUsrName.size()); //添加谁
    memcpy(pdu->caData+32,strLoginName.toStdString().c_str(),strLoginName.size()); //谁添加
    TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
    free(pdu);
    pdu = nullptr;
}



