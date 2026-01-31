#include "tcpclient.h"
#include "ui_tcpclient.h"

#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>
#include "privatechat.h"

TcpClient::TcpClient(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TcpClient)
{
    ui->setupUi(this);
    loadconfig();
    connect(&m_tcpSocket,&QTcpSocket::connected,this,&TcpClient::showConnect);
    connect(&m_tcpSocket,&QTcpSocket::readyRead,this,&TcpClient::recvMSg);

    m_tcpSocket.connectToHost(QHostAddress(m_strIP),m_usPort);

}

TcpClient::~TcpClient()
{
    delete ui;
}

void TcpClient::loadconfig()   //文件打开
{
    QFile file(":/client.txt");
    if(file.open(QIODevice::ReadOnly)){
        QByteArray itData = file.readAll();
        QString strData = itData.toStdString().c_str();
        // qDebug()<<strData;
        file.close();

        strData.replace("\r\n", " ");
        QStringList strList = strData.split(" ");

        m_strIP = strList.at(0);
        m_usPort = strList.at(1).toShort();
        qDebug()<<"ip"<<m_strIP<<"port"<<m_usPort;


    }else {
        QMessageBox::critical(this,"open config","open config error");
    }


}

void TcpClient::handleAuthResponse(PDU *pdu, const char *successMsg, const char *failMsg, const QString &title)
{
    if (strcmp(pdu->caData, successMsg) == 0) {
        QMessageBox::information(this, title, successMsg);
        if(strcmp(successMsg,LOGIN_OK) == 0){
            m_strCurPath = QString("D:/QT/code/couled_s/TcpServer/%1").arg(m_strLoginName);
            opewidget::getInstance().setLoginName(m_strLoginName);
            opewidget::getInstance().show();
            hide();
        }
    } else if (strcmp(pdu->caData, failMsg) == 0) {
        QMessageBox::warning(this, title, failMsg);
    } else {
        qWarning() << "未知响应内容:" << pdu->caData;
    }
}

TcpClient &TcpClient::getInstance()
{
    static TcpClient instance;
    return instance;
}

QTcpSocket &TcpClient::getTcpSocket()
{
    return  m_tcpSocket;
}

QString TcpClient::loginName()
{
    return m_strLoginName;
}

QString TcpClient::curPath()
{
    return m_strCurPath;
}

void TcpClient::setCurPath(QString strCurPath)
{
    m_strCurPath = strCurPath;
}

void TcpClient::showConnect()
{
    QMessageBox::information(this,"连接服务器","连接服务器成功");
}

void TcpClient::recvMSg(){
    Book *pBook = opewidget::getInstance().getBook();
    if(!opewidget::getInstance().getBook()->m_bDownload){
        qDebug() << "收到数据，可用字节数：" << m_tcpSocket.bytesAvailable();
        if(m_tcpSocket.bytesAvailable() < sizeof(uint)) {
            qDebug() << "数据不足一个PDU头部大小";
            return;
        }
        uint uiPDULen= 0 ;
        m_tcpSocket.read((char*)&uiPDULen,sizeof(uint));
        qDebug() << "PDU总长度：" << uiPDULen;
        uint uiMsgLen = uiPDULen  - sizeof(PDU);
        PDU *pdu  =  mkPDU(uiMsgLen);
        m_tcpSocket.read((char*)pdu+sizeof(uint),uiPDULen-sizeof(uint));
        qDebug() << "收到消息类型：" << pdu->uiMsgType;
        qDebug() << "caData内容：" << pdu->caData;

        switch ( pdu->uiMsgType) {
        case ENUM_MSG_TYPE_REGIST_RESPOND:
        {
            handleAuthResponse(pdu, REGIST_OK, REGIST_FAILED, tr("注册"));
            break;
        }

        case ENUM_MSG_TYPE_LOGIN_RESPOND:
        {
            handleAuthResponse(pdu, LOGIN_OK, LOGIN_FAILED, tr("登录"));
            break;
        }

        case ENUM_MSG_TYPE_ALL_ONLINE_RESPOND:
        {
            opewidget::getInstance().getFriend()->showAllOnlineUsr(pdu);
            break;
        }

        case ENUM_MSG_TYPE_SEARCH_USR_RESPOND:
        {
            if(0 == strcmp(SEARCH_USR_NO,pdu->caData)){
                QMessageBox::information(this,"search",QString("%1: not exist").arg(opewidget::getInstance().getFriend()->m_strSearchName));
            }
            else if(0 == strcmp(SEARCH_USR_ONLINE,pdu->caData)){
                QMessageBox::information(this,"search",QString("%1: online").arg(opewidget::getInstance().getFriend()->m_strSearchName));
            }
            else if(0 == strcmp(SEARCH_USR_OFFLINE,pdu->caData)){
                QMessageBox::information(this,"search",QString("%1: offline").arg(opewidget::getInstance().getFriend()->m_strSearchName));
            }
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName,pdu->caData+32,32);
            int ret = QMessageBox::information(this,"add friend",QString("%1 want to add you friend ?").arg(caName),
                                               QMessageBox::Yes,QMessageBox::No);

            PDU *respdu = mkPDU(0);
            memcpy(respdu->caData,pdu->caData,64);
            if(QMessageBox::Yes == ret){
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_AGREE;

            }else{
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REFUSE;
            }
            m_tcpSocket.write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND:
        {
            QMessageBox::information(this,"add friend",pdu->caData);
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
        {
            char caPerName[32]= {'\0'};
            memcpy(caPerName,pdu->caData,32);
            QMessageBox::information(this,"add friend","success");
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
        {
            char caPerName[32] = {'\0'};
            memcpy(caPerName,pdu->caData,32);
            QMessageBox::information(this,"add friend","nosuccess");
            break;
        }

        case ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND:
        {
            opewidget::getInstance().getFriend()->updateFriendList(pdu);
            break;
        }

        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:
        {
            char caName[32] = {'\0'};
            memcpy(caName,pdu->caData,32);
            QMessageBox::information(this,"delete friend",QString("%1 delete you").arg(caName));
            break;
        }

        case ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND:
        {
            QMessageBox::information(this,"delete friend","success");
            break;
        }

        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
        {
            if( PrivateChat::getInstance().isHidden()){
                PrivateChat::getInstance().show();
            }
            char caSendName[32] = {'\0'};
            memcpy(caSendName,pdu->caData,32);
            QString strSendName = caSendName;
            PrivateChat::getInstance().setChatName(strSendName);
            PrivateChat::getInstance().updateMsg(pdu);

            break;
        }

        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        {
            opewidget::getInstance().getFriend()->updateGroupMsg(pdu);
            break;
        }

        case ENUM_MSG_TYPE_CREATE_DIR_RESPOND:
        {
            QMessageBox::information(this,"create file",pdu->caData);
            break;
        }

        case ENUM_MSG_TYPE_FLUSH_FILE_RESPOND:{
            opewidget::getInstance().getBook()->updateFileList(pdu);
            break;
        }

        case ENUM_MSG_TYPE_DEL_DIR_RESPOND:{
            QMessageBox::information(this,"del dir",pdu->caData);
            break;
        }

        case ENUM_MSG_TYPE_RENAME_FILE_RESPOND:{
            QMessageBox::information(this,"rename dir",pdu->caData);
            if (0 == strcmp(pdu->caData, RENAME_FILE_OK)) {
                opewidget::getInstance().getBook()->flushFile();
            }
            break;
        }

        case ENUM_MSG_TYPE_ENTER_DIR_RESPOND:{
            QMessageBox::information(this,"enter dir",pdu->caData);
            break;
        }

        case ENUM_MSG_TYPE_DEL_FILE_RESPOND:{
            QMessageBox::information(this,"del file",pdu->caData);
            if (0 == strcmp(pdu->caData, DEL_FILE_OK)) {
                opewidget::getInstance().getBook()->flushFile();
            }
            break;
        }

        case ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND:{
            QMessageBox::information(this,"upload file",pdu->caData);
            if (0 == strcmp(pdu->caData, UPLOAD_FILE_OK)) {
                opewidget::getInstance().getBook()->flushFile();
            }
            break;
        }

        case ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND:{
            char caFileName[32] = {'\0'};
            qint64 fileSize = 0;
            sscanf(pdu->caData, "%s %lld", caFileName, &fileSize);

            qDebug() << "开始下载文件:" << caFileName << "大小:" << fileSize;

            if (fileSize == 0) {
                QMessageBox::warning(this, "下载失败", "服务器上找不到该文件或文件为空");
                break;
            }

            if (strlen(caFileName) > 0 && fileSize > 0) {
                pBook->m_iTotal = fileSize;
                pBook->m_iRecved = 0;
                m_file.setFileName(pBook->getSaveFilePath());
                if (!m_file.open(QIODevice::WriteOnly)) {
                    QMessageBox::warning(this, "下载失败", "无法创建本地文件，请检查保存路径");
                    pBook->setDownloadStatus(false);
                    pBook->m_iTotal = 0;
                } else {
                    pBook->setDownloadStatus(true);
                }
            } else {
                QMessageBox::warning(this, "下载失败", "服务器返回数据格式错误");
            }
            break;
        }

        case ENUM_MSG_TYPE_SHARE_FILE_NOTE:
        {
            // 收到分享通知，显示确认对话框
            char caSender[33] = {'\0'};
            char caFileName[33] = {'\0'};
            memcpy(caSender, pdu->caData, 32);
            memcpy(caFileName, pdu->caData + 32, 32);
            caFileName[31] = '\0';  // 确保终止

            if (pdu->uiMsgLen <= 1) break;  // 只有\0或无数据

            QByteArray msgBytes(pdu->uiMsgLen - 1, '\0');
            memcpy(msgBytes.data(), pdu->caMsg, pdu->uiMsgLen - 1);

            QString strMsg = QString::fromUtf8(msgBytes);
            QStringList strList = strMsg.split("|");

            if(strList.size() < 3) {
                qDebug() << "分享通知格式错误:" << strMsg;
                break;
            }

            QString strSender = strList.at(0).trimmed();
            QString strPath = strList.at(1).trimmed();
            QString strFileName = strList.at(2).trimmed();

            // 验证数据一致性
            if(strSender != QString(caSender).trimmed()) {
                qDebug() << "警告：发送者信息不一致";
                strSender = QString(caSender).trimmed();  // 以caData为准
            }
            if(strFileName != QString(caFileName).trimmed()) {
                strFileName = QString(caFileName).trimmed();  // 以caData为准
            }

            QString strInfo = QString("用户 %1 向您分享文件：%2\n路径：%3\n\n是否接收？")
                                  .arg(strSender).arg(strFileName).arg(strPath);

            int ret = QMessageBox::question(this, "收到文件分享", strInfo,
                                            QMessageBox::Yes | QMessageBox::No);

            if(ret == QMessageBox::Yes) {
                // 构造确认消息发送给服务器
                QString strRespData = QString("%1|%2").arg(strSender).arg(strPath);
                QByteArray respBytes = strRespData.toUtf8();

                PDU *respdu = mkPDU(respBytes.size() + 1);
                respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;

                memset(respdu->caData, 0, 64);

                // 前32字节：原分享者名字（服务器用来找到源文件）
                QByteArray senderBytes = strSender.toUtf8();
                int senderLen = qMin(senderBytes.size(), 31);
                memcpy(respdu->caData, senderBytes.constData(), senderLen);

                // 后32字节：文件名（必需！）
                QByteArray fileBytes = strFileName.toUtf8();
                int fileLen = qMin(fileBytes.size(), 31);
                memcpy(respdu->caData + 32, fileBytes.constData(), fileLen);

                // 消息体：分享者|原路径
                memcpy(respdu->caMsg, respBytes.constData(), respBytes.size());
                respdu->caMsg[respBytes.size()] = '\0';
                respdu->uiMsgLen = respBytes.size() + 1;

                m_tcpSocket.write((char*)respdu, respdu->uiPDULen);
                free(respdu);
            }
            break;
        }




        default:
            break;
        }
        free(pdu);
        pdu = nullptr;
    }else{
        QByteArray buffer = m_tcpSocket.readAll();
        m_file.write(buffer);
        Book *pBook = opewidget::getInstance().getBook();

        pBook->m_iRecved += buffer.size();
        if(pBook->m_iTotal == pBook->m_iRecved){
            m_file.close();
            pBook->m_iTotal = 0;
            pBook->m_iRecved = 0;
            pBook->setDownloadStatus(false);
            QMessageBox::information(this, "下载完成", "文件下载成功！");
        }else if(pBook->m_iTotal < pBook->m_iRecved){
            m_file.close();
            pBook->m_iTotal = 0;
            pBook->m_iRecved = 0;
            pBook->setDownloadStatus(false);

            QMessageBox::critical(this,"download file","failured");

        }

    }
}

void TcpClient::on_login_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();

    qDebug() << "尝试注册，用户名:" << strName << "密码:" << strPwd;

    if(!strName.isEmpty() && !strPwd.isEmpty()){
        m_strLoginName = strName;
        // 调试信息
        qDebug() << "构建注册PDU...";

        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_REQUEST;

        // 确保数据复制正确
        memset(pdu->caData, 0, 64);
        std::strncpy(pdu->caData, strName.toStdString().c_str(), 31);
        pdu->caData[31] = '\0';
        std::strncpy(pdu->caData + 32, strPwd.toStdString().c_str(), 31);
        pdu->caData[63] = '\0';

        qDebug() << "发送PDU，大小:" << pdu->uiPDULen;


        // 发送数据
        qint64 bytesWritten = m_tcpSocket.write((char*)pdu, pdu->uiPDULen);
        m_tcpSocket.flush();

        qDebug() << "已发送字节数:" << bytesWritten;
        qDebug() << "Socket状态:" << m_tcpSocket.state();


        free(pdu);
        pdu = nullptr;

    } else {
        QMessageBox::critical(this,"登录失败","登录失败:用户名或密码为空！");
    }


}

void TcpClient::on_regist_pb_clicked()
{
    QString strName = ui->name_le->text();
    QString strPwd = ui->pwd_le->text();

    qDebug() << "尝试注册，用户名:" << strName << "密码:" << strPwd;

    if(!strName.isEmpty() && !strPwd.isEmpty()){
        // 调试信息
        qDebug() << "构建注册PDU...";

        PDU *pdu = mkPDU(0);
        pdu->uiMsgType = ENUM_MSG_TYPE_REGIST_REQUEST;

        // 确保数据复制正确
        memset(pdu->caData, 0, 64); // 清空caData
        std::strncpy(pdu->caData, strName.toStdString().c_str(), 31);
        std::strncpy(pdu->caData + 32, strPwd.toStdString().c_str(), 31);

        qDebug() << "发送PDU，大小:" << pdu->uiPDULen;


        // 发送数据
        qint64 bytesWritten = m_tcpSocket.write((char*)pdu, pdu->uiPDULen);
        m_tcpSocket.flush();

        qDebug() << "已发送字节数:" << bytesWritten;
        qDebug() << "Socket状态:" << m_tcpSocket.state();

        free(pdu);
        pdu = nullptr;
    } else {
        QMessageBox::critical(this,"注册失败","注册失败:用户名或密码为空！");
    }
}

void TcpClient::on_cancel_pb_clicked()
{
    close();
}



