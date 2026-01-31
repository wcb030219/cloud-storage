#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QTcpSocket>
#include "protocol.h"
#include "opendb.h"
#include <QDir>
#include <QFile>
#include <QTimer>

class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit MyTcpSocket(QObject *parent = nullptr);
    template<typename HandlerFunc>
    void processAuthRequest(PDU *pdu,
                            unsigned int respMsgType,
                            const char* successMsg,
                            const char* failMsg,
                            HandlerFunc handler);
     QString getName() const;

signals:
    void offline(MyTcpSocket *mysocket);

public slots:
    void recvMsg();
    void clientOfflinet(); //处理客户端下线
    void sendFileToClient();
private:
    QString m_strName;
    QFile m_file;
    qint64 m_iTotal; // 文件总大小
    qint64 m_iRecved;// 已经接收多少
    bool m_bUploadt;// 状态
    QTimer *m_pTimer;
};

#endif // MYTCPSOCKET_H
