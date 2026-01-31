#include "tcpserver.h"
#include "ui_tcpserver.h"
#include "mytcpserver.h"

#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>

TcpServer::TcpServer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TcpServer)
{
    ui->setupUi(this);
    loadconfig();


    MyTcpServer::getInstance().listen(QHostAddress(m_strIP),m_usPort);
}

TcpServer::~TcpServer()
{
    delete ui;
}

void TcpServer::loadconfig()
{
    QFile file(":/server.txt");
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
