#ifndef BOOK_H
#define BOOK_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "protocol.h"
#include <QTimer>


class Book : public QWidget
{
    Q_OBJECT
public:
    explicit Book(QWidget *parent = nullptr);
    void updateFileList(const PDU *pdu);
    void setDownloadStatus(bool status);

    qint64 m_iTotal = 0; //总的文件大小
    qint64 m_iRecved = 0; //收到多少
    bool m_bDownload;
    QString getSaveFilePath();
signals:

public slots:
    void createDir();
    void flushFile();
    void delDir();
    void renameFile();
    void enterDir(const QModelIndex &index);
    void returnPre();
    void delRegFile();
    void uploadFile();
    void uploadFileData();
    void downloadFile();
    void shareFile();



private:
    QListWidget  *m_pBookListW;
    QPushButton *m_pReturnPB; //返回
    QPushButton *m_pCreateDirPB;//创建文件夹
    QPushButton *m_pDelDirPB;//删除文件夹
    QPushButton *m_pRenamePB;//重命名文件夹
    QPushButton *m_pFlushFilePB;//刷新文件夹
    QPushButton *m_pUpLoadPB;//上传文件
    QPushButton *m_pDownLoadPB;//下载文件
    QPushButton *m_pDelFilePB;//删除文件
    QPushButton *m_pShareFilePB;//共享文件

    QString m_strUploadFilePath;
    QTimer *m_pTimer;
    QString m_strSaveFilePath;



};
#endif // BOOK_H
