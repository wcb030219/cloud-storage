#include "mytcpserver.h"
#include <QDebug>

MyTcpServer::MyTcpServer() {

}

MyTcpServer &MyTcpServer::getInstance()
{
    static MyTcpServer instance;
    return instance;
}

void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "new client connected";
    MyTcpSocket *ptcpSocket = new MyTcpSocket;
    ptcpSocket->setSocketDescriptor(socketDescriptor);
    m_tcpSocketList.append(ptcpSocket);
    connect(ptcpSocket,&MyTcpSocket::offline,this,&MyTcpServer::deleteSocket);
}

void MyTcpServer::resend(const char *pername, PDU *pdu)
{
    if(nullptr == pername || nullptr == pdu){
        return;
    }
    QString strName = pername;
    for(int i = 0;i<m_tcpSocketList.size();i++){
        if(strName == m_tcpSocketList.at(i)->getName()){
            m_tcpSocketList.at(i)->write((char*)pdu,pdu->uiPDULen);
        }
    }
}

void MyTcpServer::deleteSocket(MyTcpSocket *mysocket)
{
    QList<MyTcpSocket*>::Iterator iter = m_tcpSocketList.begin();
    for(;iter!= m_tcpSocketList.end();iter++){
        if(mysocket == *iter){
            (*iter)->deleteLater();
            *iter = NULL;
            m_tcpSocketList.erase(iter);
            break;
        }
    }
}
