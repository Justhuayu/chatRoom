#include "tcpConnect.h"
#include <QHostAddress>
#include <QDebug>

TcpConnect::TcpConnect(QObject *parent)
    : QObject(parent),
    m_socket(new QTcpSocket(this))
{
    m_tcp_isConnect = false;
    connect(m_socket,SIGNAL(connected()),this,SLOT(m_tcp_connected()));
    connect(m_socket,SIGNAL(errorOccurred(QAbstractSocket::SocketError)),this,
            SLOT(m_tcp_connect_error(QAbstractSocket::SocketError socketError)));
    connect(m_socket,SIGNAL(disconnected()),this,SLOT(m_tcp_disconnected()));
}

//连接服务器
void TcpConnect::connectToServer()
{
    m_socket->connectToHost(QHostAddress(this->peer_ip),peer_port);
    //等待连接超过1s，认为端口号/ip错误，提示错误
    if (!m_socket->waitForConnected(1000)) {
        emit tcp_connect_error(m_socket->errorString());
    }
}

//断开服务器
void TcpConnect::disconnectToServer()
{
    m_socket->disconnectFromHost();
}

//通知chatWidget连接成功
void TcpConnect::m_tcp_connected(){
    m_tcp_isConnect = true;
    emit tcp_connected();
}
void TcpConnect::m_tcp_disconnected(){
    m_tcp_isConnect = false;
    emit tcp_disconnected();
}
//通知chatWidget连接失败
void TcpConnect::m_tcp_connect_error(QAbstractSocket::SocketError socketError){
    Q_UNUSED(socketError);
    m_tcp_isConnect =false;
    emit tcp_connect_error(m_socket->errorString());
}
