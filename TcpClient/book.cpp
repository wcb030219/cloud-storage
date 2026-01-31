#include "book.h"
#include "tcpclient.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include "opewidget.h"
#include "sharefile.h"



Book::Book(QWidget *parent)
    : QWidget{parent}
{
    m_bDownload = false;
    m_pTimer = new QTimer;

    m_pBookListW = new QListWidget;
    m_pReturnPB = new QPushButton("返回");
    m_pCreateDirPB = new QPushButton("创建文件夹");
    m_pDelDirPB = new QPushButton("删除文件夹");
    m_pRenamePB = new QPushButton("重命名文件夹");
    m_pFlushFilePB = new QPushButton("刷新文件夹");

    QVBoxLayout *pDirVBL = new QVBoxLayout;
    pDirVBL->addWidget(m_pReturnPB);
    pDirVBL->addWidget(m_pCreateDirPB);
    pDirVBL->addWidget(m_pDelDirPB);
    pDirVBL->addWidget(m_pRenamePB);
    pDirVBL->addWidget(m_pFlushFilePB);

    m_pUpLoadPB = new QPushButton("上传文件");
    m_pDownLoadPB = new QPushButton("下载文件");
    m_pDelFilePB = new QPushButton("删除文件");
    m_pShareFilePB = new QPushButton("共享文件");

    QVBoxLayout *pFileVBL = new QVBoxLayout;

    pFileVBL->addWidget(m_pUpLoadPB);
    pFileVBL->addWidget(m_pDownLoadPB);
    pFileVBL->addWidget(m_pDelFilePB);
    pFileVBL->addWidget(m_pShareFilePB);

    QHBoxLayout *pMain = new QHBoxLayout;
    pMain->addWidget(m_pBookListW);
    pMain->addLayout(pDirVBL);
    pMain->addLayout(pFileVBL);

    setLayout(pMain);

    connect(m_pCreateDirPB,&QPushButton::clicked,this,&Book::createDir);
    connect(m_pFlushFilePB,&QPushButton::clicked,this,&Book::flushFile);
    connect(m_pDelDirPB,&QPushButton::clicked,this,&Book::delDir);
    connect(m_pRenamePB,&QPushButton::clicked,this,&Book::renameFile);
    connect(m_pBookListW,&QListWidget::doubleClicked,this,&Book::enterDir);
    connect(m_pReturnPB,&QPushButton::clicked,this,&Book::returnPre);
    connect(m_pDelFilePB,&QPushButton::clicked,this,&Book::delRegFile);
    connect(m_pUpLoadPB,&QPushButton::clicked,this,&Book::uploadFile);
    connect(m_pTimer,&QTimer::timeout,this,&Book::uploadFileData);
    connect(m_pDownLoadPB,&QPushButton::clicked,this,&Book::downloadFile);
    connect(m_pShareFilePB,&QPushButton::clicked,this,&Book::shareFile);

}


void Book::updateFileList(const PDU *pdu)
{
    if(nullptr == pdu){
        return;
    }
    m_pBookListW->clear();
    FileInfo *pFileInfo = nullptr;
    int iCount = pdu->uiMsgLen/sizeof(FileInfo);
    for(int i = 0;i<iCount;i++){
        pFileInfo = (FileInfo*)(pdu->caMsg)+i;
        QListWidgetItem *pItem = new QListWidgetItem;
        if(0 == pFileInfo->iFileType){
            pItem->setIcon(QIcon(QPixmap(":/map/reg.png")));
        }else if(1 == pFileInfo->iFileType){
            pItem->setIcon(QIcon(QPixmap(":/map/dir.png")));
        }
        pItem->setText(pFileInfo->caFileName);
        pItem->setData(Qt::UserRole, pFileInfo->iFileType);
        m_pBookListW->addItem(pItem);
    }
}

void Book::setDownloadStatus(bool status)
{
    m_bDownload = status;
}

QString Book::getSaveFilePath()
{
    return m_strSaveFilePath;
}

void Book::createDir()
{
    QString strNewDir = QInputDialog::getText(this,"create dir","new dir name");
    if(!strNewDir.isEmpty()){
        if(strNewDir.size()>32){
            QMessageBox::warning(this,"create dir","name is not 32");
        }
        else{
            QString strName = TcpClient::getInstance().loginName();
            QString strCurPath = TcpClient::getInstance().curPath();
            PDU *pdu = mkPDU(strCurPath.size()+1);
            pdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_REQUEST;
            strncpy(pdu->caData, strName.toStdString().c_str(), 32);
            pdu->caData[31] = '\0';
            strncpy(pdu->caData + 32, strNewDir.toStdString().c_str(), 32);
            pdu->caData[63] = '\0';
            strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
            pdu->caMsg[strCurPath.size()] = '\0';
            TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
            free(pdu);
            pdu = nullptr;

        }
    }else {
        QMessageBox::warning(this,"create dir","name is not exist");
    }

}

void Book::flushFile()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    PDU *pdu = mkPDU(strCurPath.size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
    strncpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
    free(pdu);
    pdu = nullptr;

}

void Book::delDir()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem =  m_pBookListW->currentItem();
    if(nullptr == pItem){
        QMessageBox::warning(this,"del dir","choose dir");
    }else{
        QString strDelName = pItem->text();
        PDU *pdu = mkPDU(strCurPath.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_REQUEST;
        strncpy(pdu->caData,strDelName.toStdString().c_str(),strDelName.size());
        memcpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu = nullptr;
    }

}

void Book::renameFile()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem =  m_pBookListW->currentItem();
    if(nullptr == pItem){
        QMessageBox::warning(this,"rename dir","choose dir");
    }else{
        QString strOldName = pItem->text();
        QString strNewName = QInputDialog::getText(this,"rename file","new filename");
        if(!strNewName.isEmpty()){
            PDU *pdu = mkPDU(strCurPath.size()+1);
            pdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_REQUEST;
            strncpy(pdu->caData,strOldName.toStdString().c_str(),strOldName.size());
            strncpy(pdu->caData+32,strNewName.toStdString().c_str(),strNewName.size());
            memcpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
            TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
            free(pdu);
            pdu = nullptr;
        }else{
            QMessageBox::warning(this,"rename dir","is not null");
        }
    }
}

// void Book::enterDir(const QModelIndex &index)
// {
//     QString strDirName  = index.data().toString();
//     QString strCurPath = TcpClient::getInstance().curPath();
//     QString strNewPath = QString("%1%2").arg(strCurPath).arg(strDirName);
//     TcpClient::getInstance().setCurPath(strNewPath);
//     PDU *pdu = mkPDU(strCurPath.size()+1);
//     pdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_REQUEST;
//     strncpy(pdu->caData,strDirName.toStdString().c_str(),strDirName.size());
//     memcpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
//     TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
//     free(pdu);
//     pdu = nullptr;
// }

void Book::enterDir(const QModelIndex &index)
{
    // 获取文件类型，1表示文件，0表示文件夹
    int fileType = index.data(Qt::UserRole).toInt();
    if(fileType == 1) {
        // 双击的是文件，不进入
        QMessageBox::information(this, "提示", "这是一个文件，无法进入");
        return;
    }

    QString strDirName = index.data().toString();
    QString strCurPath = TcpClient::getInstance().curPath();

    // 统一路径分隔符，并确保末尾有 /
    strCurPath = strCurPath.replace("\\", "/");
    if (!strCurPath.endsWith("/")) {
        strCurPath += "/";
    }

    // 拼接新路径
    QString strNewPath = strCurPath + strDirName;
    TcpClient::getInstance().setCurPath(strNewPath);

    // 发送进入目录请求，传递当前路径和目录名
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_REQUEST;
    strncpy(pdu->caData, strDirName.toStdString().c_str(), 32);
    pdu->caData[31] = '\0';
    memcpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    pdu->caMsg[strCurPath.size()] = '\0';

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = nullptr;
}

// void Book::returnPre()
// {
//     QString strCurPath = TcpClient::getInstance().curPath();
//     QString strLoginName = TcpClient::getInstance().loginName();
//     QString strRootPath = QString("D:/QT/code/couled_s/TcpServer/%1").arg(strLoginName);
//     if(strCurPath == strRootPath){
//         QMessageBox::warning(this,"return","is first");
//     }else{
//         int index = strCurPath.lastIndexOf('/');
//         if(index != -1){
//         strCurPath.remove(index,strCurPath.size()-index);
//         TcpClient::getInstance().setCurPath(strCurPath);
//         flushFile();
//         }else{
//             QMessageBox::warning(this,"return","is not first");
//         }
//     }
// }

void Book::returnPre()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QString strLoginName = TcpClient::getInstance().loginName();
    QString strRootPath = QString("D:/QT/code/couled_s/TcpServer/%1").arg(strLoginName);

    // 统一路径分隔符，去除尾部斜杠（如果有）
    strCurPath = strCurPath.replace("\\", "/");
    if (strCurPath.endsWith("/")) {
        strCurPath.chop(1);
    }

    // 检查是否是根目录
    if(strCurPath == strRootPath){
        QMessageBox::warning(this, "返回", "已经是根目录了");
        return;
    }

    // 找到最后一个 / 的位置
    int index = strCurPath.lastIndexOf('/');
    if(index != -1 && index > 0){
        // 截取父目录路径
        strCurPath = strCurPath.left(index);
        TcpClient::getInstance().setCurPath(strCurPath);
        flushFile();  // 刷新文件列表
    } else {
        QMessageBox::warning(this, "返回", "无法返回上一级");
    }
}

void Book::delRegFile()
{
    QString strCurPath = TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL == pItem){
        QMessageBox::information(this,"del file","choos file");
    }else{
        QString strDelName = pItem->text();
        PDU *pdu = mkPDU(strCurPath.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_REQUEST;
        strncpy(pdu->caData,strDelName.toStdString().c_str(),strDelName.size());
        memcpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu = nullptr;
    }
}

void Book::uploadFile()
{
    m_strUploadFilePath = QFileDialog::getOpenFileName();
    if(!m_strUploadFilePath.isEmpty()){
        int index  = m_strUploadFilePath.lastIndexOf("/");
        QString strFileName = m_strUploadFilePath.right(m_strUploadFilePath.size()-index-1);
        QFile file(m_strUploadFilePath);
        qint64 filesize = file.size();//file size
        QString strCurPath = TcpClient::getInstance().curPath();
        PDU *pdu = mkPDU(strCurPath.size()+1);
        pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
        memcpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());
        sprintf(pdu->caData,"%s %lld",strFileName.toStdString().c_str(),filesize);
        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu = nullptr;
        m_pTimer->start(1000);
    }else{
        QMessageBox::warning(this,"upload file","file name is not null");
    }

}

void Book::uploadFileData()
{
    //m_pTimer->stop();
    QFile file(m_strUploadFilePath);
    if(!file.open(QIODevice::ReadOnly)){
        QMessageBox::warning(this,"upload file","file not open");
        return;
    }
    char *pBuffer = new char[4096];
    qint64 ret = 0;
    while(true){
        ret = file.read(pBuffer,4096);
        if(ret > 0&&ret <= 4096){
            TcpClient::getInstance().getTcpSocket().write(pBuffer,ret);
        }else if(0 == ret){
            break;
        }else{
            QMessageBox::warning(this,"upload file","read not ");
            break;
        }
    }
    file.close();
    delete[]pBuffer;
    pBuffer = nullptr;
}

void Book::downloadFile()
{
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL == pItem){
        QMessageBox::information(this,"download file","请选择要下载的文件");
        return;
    }
    QString strSaveFilePath = QFileDialog::getSaveFileName(this, "保存文件", pItem->text());
    if(strSaveFilePath.isEmpty()){
        return;
    }

    // 设置保存路径
    m_strSaveFilePath = strSaveFilePath;

    // 重置下载状态
    m_iTotal = 0;
    m_iRecved = 0;
    m_bDownload = false;

    QString strCurPath = TcpClient::getInstance().curPath();
    PDU *pdu = mkPDU(strCurPath.size()+1);
    pdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST;
    QString strFileName = pItem->text();
    strcpy(pdu->caData, strFileName.toStdString().c_str());
    memcpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    pdu->caMsg[strCurPath.size()] = '\0';

    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = nullptr;
}

void Book::shareFile()
{
    // 1. 获取当前选中的文件
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(nullptr == pItem) {
        QMessageBox::warning(this, "分享文件", "请先选择要分享的文件");
        return;
    }

    // 2. 获取文件名和当前路径
    QString strFileName = pItem->text();
    QString strCurPath = TcpClient::getInstance().curPath();

    // 3. 获取好友列表（从 Friend 类）
    QListWidget *pFriendList = opewidget::getInstance().getFriend()->getFriendList();
    if(pFriendList->count() == 0) {
        QMessageBox::information(this, "分享文件", "您还没有好友，无法分享");
        return;
    }

    // 4. 设置分享窗口信息并显示
    ShareFile::getInstance().setFileInfo(strCurPath, strFileName);
    ShareFile::getInstance().updateFriend(pFriendList);
    ShareFile::getInstance().show();
}








