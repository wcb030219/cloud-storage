#include "mytcpsocket.h"
#include <QtGlobal>
#include "mytcpserver.h"
#include <QDir>
#include <QFileInfoList>


MyTcpSocket::MyTcpSocket(QObject *parent)
    : QTcpSocket{parent}
{
    OpenDB::getInstance().init();
    connect(this,&QTcpSocket::readyRead,this,&MyTcpSocket::recvMsg);
    connect(this,&QTcpSocket::disconnected,this,&MyTcpSocket::clientOfflinet);

    m_bUploadt = false;
    m_pTimer = new QTimer;
    connect(m_pTimer,&QTimer::timeout,this,&MyTcpSocket::sendFileToClient);

}

QString MyTcpSocket::getName() const
{
    return m_strName;
}

// 在类定义中添加模板成员函数
template<typename HandlerFunc>
void MyTcpSocket::processAuthRequest(PDU *pdu,
                        unsigned int respMsgType,
                        const char* successMsg,
                        const char* failMsg,
                        HandlerFunc handler)
{
    char caName[32] = {'\0'};
    char caPwd[32] = {'\0'};

    // 通用解析逻辑
    std::memcpy(caName, pdu->caData, 31);  // 只复制31字节
    std::memcpy(caPwd, pdu->caData + 32, 31);
    caName[31] = '\0';
    caPwd[31] = '\0';

    qDebug() << "解析用户名:" << caName;
    qDebug() << "解析密码:" << caPwd;

    // 调用具体的业务处理函数
    bool ret = handler(caName, caPwd);

    // 通用响应逻辑
    PDU *respdu = mkPDU(0);
    respdu->uiMsgType = respMsgType;
    if(ret){
        qstrncpy(respdu->caData,successMsg,sizeof(respdu->caData));
        if(pdu->uiMsgType == ENUM_MSG_TYPE_LOGIN_REQUEST){
            m_strName = caName;
        }
        if(pdu->uiMsgType == ENUM_MSG_TYPE_REGIST_REQUEST){
            QDir dir;
            dir.mkdir(QString("D:/QT/code/couled_s/TcpServer/%1").arg(caName));
        }
    }else{
         qstrncpy(respdu->caData,failMsg,sizeof(respdu->caData));
    }
    respdu->caData[sizeof(respdu->caData) - 1] = '\0';  // 确保终止

    qDebug() << "发送响应:" << (ret ? successMsg : failMsg);
    qDebug() << "响应PDU大小:" << respdu->uiPDULen;

    this->write((char*)respdu, respdu->uiPDULen);
    this->flush();
    free(respdu);
    respdu=NULL;
}



void MyTcpSocket::recvMsg()
{
    if(!m_bUploadt){

        qDebug() << "收到数据，可用字节数:" << this->bytesAvailable();

        uint uiPDULen= 0;
        this->read((char*)&uiPDULen, sizeof(uint));
        qDebug() << "PDU总长度:" << uiPDULen;

        uint uiMsgLen = uiPDULen - sizeof(PDU);
        PDU *pdu = mkPDU(uiMsgLen);
        this->read((char*)pdu + sizeof(uint), uiPDULen - sizeof(uint));

        qDebug() << "收到消息类型:" << pdu->uiMsgType;
        qDebug() << "caData内容:" << pdu->caData;

        switch(pdu->uiMsgType) {
        case ENUM_MSG_TYPE_REGIST_REQUEST:
        {
            processAuthRequest(pdu,
                               ENUM_MSG_TYPE_REGIST_RESPOND,
                               REGIST_OK,
                               REGIST_FAILED,
                               [](const char* name, const char* pwd) {
                                   return OpenDB::getInstance().handleRegist(name, pwd);
                               });
            break;
        }

        case ENUM_MSG_TYPE_LOGIN_REQUEST:
        {
            processAuthRequest(pdu,
                               ENUM_MSG_TYPE_LOGIN_RESPOND,
                               LOGIN_OK,
                               LOGIN_FAILED,
                               [](const char* name, const char* pwd) {
                                   return OpenDB::getInstance().handleLogin(name, pwd);
                               });
            break;
        }

        case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:
        {
            QStringList ret = OpenDB::getInstance().handleALLOnline();
            uint uiMsgLen = ret.size()*32;
            PDU *respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
            for(int i =0;i<ret.size();i++){
                memcpy(respdu->caMsg+i*32,ret.at(i).toStdString().c_str(),ret.at(i).size());
            }
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:
        {
            int ret = OpenDB::getInstance().handleSearchUsr(pdu->caData);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
            if(-1 == ret){
                strcpy(respdu->caData,SEARCH_USR_NO);
            }else if(1== ret){
                strcpy(respdu->caData,SEARCH_USR_ONLINE);
            }else if(0== ret){
                strcpy(respdu->caData,SEARCH_USR_OFFLINE);
            }
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        {
            char caPerName[32] = {'\0'};
            char caName[32] = {'\0'};

            // 通用解析逻辑
            std::memcpy(caPerName, pdu->caData, 31);  // 只复制31字节
            std::memcpy(caName, pdu->caData + 32, 31);
            caPerName[31] = '\0';
            caName[31] = '\0';
            int ret = OpenDB::getInstance().handleAddFriend(caPerName,caName);
            PDU *respdu = nullptr;

            if(-1 == ret){
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,UNKNOW_ERROR);
                this->write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }else if(0 == ret){
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,EXISTED_FRIEND);
                this->write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }else if(1 == ret){
                MyTcpServer::getInstance().resend(caPerName,pdu);


            }else if(2 == ret){
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,ADD_FRIEND_OFFLINE);
                this->write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu=NULL;

            }else if(3 == ret){
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData,ADD_FRIEND_NOEXIST);
                this->write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
        {
            char caPerName[32]={'\0'};
            char caName[32]={'\0'};
            strncpy(caPerName,pdu->caData,32);
            strncpy(caName,pdu->caData+32,32);
            OpenDB::getInstance().handleAgreeAddFriend(caPerName,caName);
            MyTcpServer::getInstance().resend(caName,pdu);
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
        {
            char caName[32] = {'\0'};
            memcpy(caName,pdu->caData+32,32);
            MyTcpServer::getInstance().resend(caName,pdu);
            break;
        }

        case ENUM_MSG_TYPE_FlUSH_FRIEND_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName,pdu->caData,32);
            QStringList ret =OpenDB::getInstance().handleFlushFriend(caName);
            uint uiMsgLen = ret.size()*32;
            PDU *respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
            for (int i =0;i<ret.size();i++){
                memcpy((char*)respdu->caMsg+i*32,ret.at(i).toStdString().c_str(),ret.at(i).size());
            }
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:
        {
            char caSelfName[32] = {'\0'};
            char caFriendName[32] = {'\0'};
            strncpy(caSelfName,pdu->caData,32);
            strncpy(caFriendName,pdu->caData+32,32);
            OpenDB::getInstance().handleDelFriend(caSelfName,caFriendName);
            PDU *respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType=ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
            strcpy(respdu->caData,DELETE_FRIEND_OK);
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            MyTcpServer::getInstance().resend(caFriendName,pdu);
            break;

        }

        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
        {
            char caPerName[32] = {'\0'};

            memcpy(caPerName,pdu->caData+32,32);
            MyTcpServer::getInstance().resend(caPerName,pdu);


            break;
        }

        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName,pdu->caData,32);
            QStringList onlineFriend =OpenDB::getInstance().handleFlushFriend(caName);
            QString tmp;
            for(int i =0;i<onlineFriend.size();i++){
                tmp = onlineFriend.at(i);
                MyTcpServer::getInstance().resend(tmp.toStdString().c_str(),pdu);
            }
            break;
        }

        case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:{
            QDir dir;
            pdu->caMsg[pdu->uiMsgLen - 1] = '\0';
            QString strCurPath = QString("%1").arg(pdu->caMsg);
            qDebug() << "解析出的当前路径:" << strCurPath;
            bool ret = dir.exists(QString(strCurPath));
            PDU *respdu = nullptr;
            if(ret){ // dir is exist
                char caNewDir[32] = {'\0'};
                memcpy(caNewDir,pdu->caData+32,32);
                caNewDir[31] = '\0';
                QString strNewPath  = strCurPath+"/"+caNewDir;
                ret = dir.exists(strNewPath);
                if(ret){ //create file is exist
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData,FILE_NAME_EXIST);
                }else{  //create file not exist
                    dir.mkdir(strNewPath);
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData,CREATE_DIR_OK);
                }
            }else{ //dir is not exist
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                strcpy(respdu->caData,DIR_NOT_EXIST);

            }
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:
        {
            char *pCurPath = new char[pdu->uiMsgLen];
            memcpy(pCurPath,pdu->caMsg,pdu->uiMsgLen);
            QDir dir(pCurPath);
            QFileInfoList fileInfoList = dir.entryInfoList();
            int iFileCount  = fileInfoList.size();
            PDU *respdu = mkPDU(sizeof(FileInfo)*(iFileCount));
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
            FileInfo *pFileInfo = nullptr;
            QString strFileName;
            for(int i =0;i<iFileCount;i++){
                pFileInfo = (FileInfo*)(respdu->caMsg)+i;
                strFileName = fileInfoList[i].fileName();

                memcpy(pFileInfo->caFileName,strFileName.toStdString().c_str(),strFileName.size());
                if(fileInfoList[i].isDir()){
                    pFileInfo->iFileType = 0;
                }
                else if(fileInfoList[i].isFile()){
                    pFileInfo->iFileType = 1;
                }
            }
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_DEL_DIR_REQUEST:{
            char caName[32] = {'\0'};
            strcpy(caName,pdu->caData);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caName);

            QFileInfo fileInfo(strPath);
            bool ret = false;
            if(fileInfo.isDir()){
                QDir dir;
                dir.setPath(strPath);
                ret = dir.removeRecursively();
            }else if(fileInfo.isFile()){
                ret = false;
            }
            PDU *respdu = nullptr;
            if(ret){
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData,DEL_DIR_OK, strlen(DEL_DIR_OK));
            }else{
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData,DEL_DIR_FAILURED, strlen(DEL_DIR_FAILURED));
            }
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_RENAME_FILE_REQUEST:{
            char caOldName[32] = {'\0'};
            char caNewName[32] = {'\0'};
            strncpy(caOldName,pdu->caData,32);
            strncpy(caNewName,pdu->caData+32,32);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            QString strOldPath = QString("%1/%2").arg(pPath).arg(caOldName);
            QString strNewPath = QString("%1/%2").arg(pPath).arg(caNewName);
            QDir dir;
            bool ret = dir.rename(strOldPath,strNewPath);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_RESPOND;
            if(ret){
                strcpy(respdu->caData,RENAME_FILE_OK);
            }else{
                strcpy(respdu->caData,RENAME_FILE_FAILURED);
            }
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:{
            char caEnterName[32] = {'\0'};
            strncpy(caEnterName,pdu->caData,32);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caEnterName);
            PDU *respdu = NULL;
            QFileInfo fileInfo(strPath);
            if(fileInfo.isDir()){
                QDir dir(strPath);
                QFileInfoList fileInfoList = dir.entryInfoList();
                int iFileCount  = fileInfoList.size();
                respdu = mkPDU(sizeof(FileInfo)*(iFileCount));
                respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
                FileInfo *pFileInfo = nullptr;
                QString strFileName;
                for(int i =0;i<iFileCount;i++){
                    pFileInfo = (FileInfo*)(respdu->caMsg)+i;
                    strFileName = fileInfoList[i].fileName();

                    memcpy(pFileInfo->caFileName,strFileName.toStdString().c_str(),strFileName.size());
                    if(fileInfoList[i].isDir()){
                        pFileInfo->iFileType = 0;
                    }
                    else if(fileInfoList[i].isFile()){
                        pFileInfo->iFileType = 1;
                    }
                }
                this->write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else if(fileInfo.isFile()){
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
                strcpy(respdu->caData,ENTER_DIR_FAILURED);
                this->write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            break;
        }

        case ENUM_MSG_TYPE_DEL_FILE_REQUEST:{
            char caName[32] = {'\0'};
            strcpy(caName,pdu->caData);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caName);

            QFileInfo fileInfo(strPath);
            bool ret = false;
            if(fileInfo.isDir()){
                ret = false;
            }else if(fileInfo.isFile()){
                QDir dir;
                ret = dir.remove(strPath);

            }
            PDU *respdu = nullptr;
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
            if(ret){
                strcpy(respdu->caData,DEL_FILE_OK);
            }else{
                strcpy(respdu->caData,DEL_FILE_FAILURED);
            }
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;

        }

        case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:{
            char caFileName[32] = {'\0'};
            qint64 filesize = 0;
            sscanf(pdu->caData,"%s %lld",caFileName,&filesize);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            delete []pPath;
            pPath = nullptr;

            m_file.setFileName(strPath);
            //只写的方式打开，若不存在，自动创建
            if(m_file.open(QIODevice::WriteOnly)){
                m_bUploadt = true;   //可以多线程
                m_iTotal = filesize;
                m_iRecved = 0;
            }
            break;
        }

        case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:{
            char caFileName[32] = {'\0'};
            strncpy(caFileName, pdu->caData, 31);
            caFileName[31] = '\0';

            char *pPath = new char[pdu->uiMsgLen + 1];
            memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            pPath[pdu->uiMsgLen] = '\0';

            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            delete[] pPath;
            pPath = nullptr;

            QFileInfo fileInfo(strPath);
            qint64 filesize = fileInfo.size();

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;

            if (!fileInfo.exists() || !fileInfo.isFile()) {
                sprintf(respdu->caData, "%s %lld", caFileName, 0);
                this->write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                qDebug() << "下载请求失败：文件不存在" << strPath;
                break;
            }

            // 文件存在，发送文件信息（大小 > 0）
            sprintf(respdu->caData, "%s %lld", caFileName, filesize);
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;

            // 准备发送文件数据（移到 if 外面，且不要 break）
            m_file.setFileName(strPath);
            if (!m_file.open(QIODevice::ReadOnly)) {
                qDebug() << "无法打开文件进行读取:" << strPath;
                break;
            }

            // 启动定时器发送文件内容
            m_pTimer->start(1000);
            break;
        }

        case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:
        {
            // 格式: caData前32字节是发送者, caMsg是 接收者1,接收者2,...|路径|文件名
            char caSender[33] = {'\0'};
            memcpy(caSender, pdu->caData, 32);

            QString strMsgContent = QString::fromUtf8(pdu->caMsg);
            QStringList parts = strMsgContent.split("|");

            if(parts.size() < 3) {
                qDebug() << "分享文件请求格式错误";
                break;
            }

            QString strReceivers = parts.at(0);    // 接收者列表，逗号分隔
            QString strPath = parts.at(1);         // 文件所在路径
            QString strFileName = parts.at(2);     // 文件名

            QStringList receiverList = strReceivers.split(",");

            // 构造转发消息（发送给接收者的通知）
            QString strNoteMsg = QString("%1|%2|%3").arg(caSender).arg(strPath).arg(strFileName);
            QByteArray noteBytes = strNoteMsg.toUtf8();

            // 给每个接收者发送分享通知
            for(const QString& receiver : receiverList) {
                QString strReceiver = receiver.trimmed();
                if(strReceiver.isEmpty()) continue;

                PDU *notePdu = mkPDU(noteBytes.size() + 1);
                notePdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE;

                // caData前32字节是发送者，后32字节是文件名
                memset(notePdu->caData, 0, 64);
                QByteArray senderByte = QString(caSender).toUtf8();
                QByteArray fileByte = strFileName.toUtf8();
                memcpy(notePdu->caData, senderByte.constData(), qMin(senderByte.size(), 31));
                memcpy(notePdu->caData + 32, fileByte.constData(), qMin(fileByte.size(), 31));

                // caMsg是 发送者|路径|文件名
                memcpy(notePdu->caMsg, noteBytes.constData(), noteBytes.size());
                notePdu->caMsg[noteBytes.size()] = '\0';
                notePdu->uiMsgLen = noteBytes.size() + 1;

                // 转发给对应客户端
                MyTcpServer::getInstance().resend(strReceiver.toStdString().c_str(), notePdu);
                free(notePdu);
            }

            // 给发送者返回"分享请求已发送"的确认
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;  // 复用类型或新增类型
            strcpy(respdu->caData, "分享请求已发送");
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            break;
        }

        case ENUM_MSG_TYPE_SHARE_FILE_RESPOND:
        {
            // 格式: caData前32字节是原分享者，后32字节是文件名；caMsg是 分享者|原路径
            char caSender[33] = {'\0'};
            char caFileName[33] = {'\0'};
            memcpy(caSender, pdu->caData, 32);
            memcpy(caFileName, pdu->caData + 32, 32);

            QString strMsg = QString::fromUtf8(pdu->caMsg);
            QStringList parts = strMsg.split("|");

            if(parts.size() < 2) {
                qDebug() << "分享确认消息格式错误:" << strMsg;
                break;
            }

            QString strOriginalSender = parts.at(0);  // 应与caSender一致
            QString strOriginalPath = parts.at(1);    // 原文件所在路径

            // 构造源文件路径（分享者的文件）
            QString strSrcFile = QString("%1/%2").arg(strOriginalPath).arg(caFileName);
            // 构造目标路径（当前用户m_strName的目录）
            QString strDstDir = QString("D:/QT/code/couled_s/TcpServer/%1").arg(m_strName);
            QString strDstFile = QString("%1/%2").arg(strDstDir).arg(caFileName);

            qDebug() << "分享文件：从" << strSrcFile << "复制到" << strDstFile;

            // 确保目标目录存在
            QDir dir;
            if(!dir.exists(strDstDir)) {
                dir.mkpath(strDstDir);
            }

            // 执行拷贝
            bool success = QFile::copy(strSrcFile, strDstFile);

            // 通知原分享者结果
            PDU *notifyPdu = mkPDU(64);
            notifyPdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
            memset(notifyPdu->caData, 0, 64);

            // 前32字节：接收者名字（当前用户），后32字节：文件名
            QByteArray recvBytes = m_strName.toUtf8();
            QByteArray fileBytes = QString(caFileName).toUtf8();
            memcpy(notifyPdu->caData, recvBytes.constData(), qMin(recvBytes.size(), 31));
            memcpy(notifyPdu->caData + 32, fileBytes.constData(), qMin(fileBytes.size(), 31));

            strcpy(notifyPdu->caMsg, success ? "success" : "failed");

            // 发送给原分享者
            MyTcpServer::getInstance().resend(caSender, notifyPdu);
            free(notifyPdu);

            // 给接收者返回操作结果
            PDU *localResp = mkPDU(0);
            localResp->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
            strcpy(localResp->caData, success ? "文件接收成功" : "文件接收失败");
            this->write((char*)localResp, localResp->uiPDULen);
            free(localResp);
            break;
        }

        default:
            qDebug() << "未知消息类型:" << pdu->uiMsgType;
            break;
        }

        free(pdu);
        pdu = nullptr;
    }
    else{
        PDU *respdu = nullptr;
        respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
        QByteArray buff = readAll();
        m_file.write(buff);
        m_iRecved+=buff.size();
        if(m_iTotal == m_iRecved){
            m_file.close();
            m_bUploadt = false;
            strcpy(respdu->caData,UPLOAD_FILE_OK);
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
        }else if(m_iTotal < m_iRecved){
            m_file.close();
            m_bUploadt = false;
            strcpy(respdu->caData,UPLOAD_FILE_FAILURED);
            this->write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu=NULL;

        }
    }
}

void MyTcpSocket::clientOfflinet()
{
    OpenDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this);
}

void MyTcpSocket::sendFileToClient()
{
    m_pTimer->stop();
    char *pData = new char[4096];
    qint64 ret = 0;
    while(true){
        ret = m_file.read(pData,4096);
        if(ret > 0 && ret<=4096){
            write(pData,ret);
            this->waitForBytesWritten(10);
        }else if(0 == ret){
            m_file.close();
            break;
        }else if(ret < 0){
            qDebug() << "发送文件读取出错";
            m_file.close();
            break;
        }
    }
    delete []pData;
    pData = nullptr;
}

