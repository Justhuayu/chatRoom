#include "tcpConnect.h"
#include <QHostAddress>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
TcpConnect::TcpConnect(QObject *parent)
    : QObject(parent),
    m_socket(new QTcpSocket(this))
{
    tcp_isConnect = false;
    connect(m_socket,SIGNAL(connected()),this,SLOT(m_tcp_connected()));
    connect(m_socket,SIGNAL(errorOccurred(QAbstractSocket::SocketError)),this,
            SLOT(m_tcp_connect_error(QAbstractSocket::SocketError socketError)));
    connect(m_socket,SIGNAL(disconnected()),this,SLOT(m_tcp_disconnected()));
}
tcp_protocol::communication_head TcpConnect::recvData(){
    //1.检查tcp连接状态
    if(!this->tcp_isConnect){
        qDebug()<<"tcp disconnect!";
    }
    //2.阻塞等待
    //等待数据发送完毕
    if(!m_socket->waitForBytesWritten()){
        qDebug()<<"data send error!";
    }
    //等待3秒回应
    if(!m_socket->waitForReadyRead(3000)){
        qDebug()<<"no response reveived from server, timeout!";
    }
    //3.循环读取head数据
    tcp_protocol::communication_head head;
    QByteArray res_data;
    while(res_data.size() < static_cast<qsizetype>(sizeof(head))){
        QByteArray tmp = m_socket->read(sizeof(head) - res_data.size());
        if (tmp.isEmpty()) {
            qDebug() << "Error reading data.";
            throw std::runtime_error("Error reading data");
        }
         res_data.append(tmp);
    }
    if (res_data.size() != sizeof(head)) {
         qDebug() << "Not enough data received.";
         throw std::runtime_error("Not enough data received");
    }
    std::memcpy(&head, res_data.data(), sizeof(head));


    return head;
}
//发送二进制数据到后端
bool TcpConnect::sendData(const tcp_protocol::communication_head &head,const char* data){
    //1.检查tcp连接状态
    if(!this->tcp_isConnect){
        qDebug()<<"tcp disconnect!";
        return false;
    }
    QByteArray send_buffer;
    send_buffer.append(reinterpret_cast<const char*>(&head), sizeof(head));
    send_buffer.append(data,buffer_sizes::LOGIN_BUFFER_SIZE);
    //tcp socket发送
    if(-1 == m_socket->write(send_buffer)){
        qDebug()<<"tcp send data error: "<<m_socket->error();
        return false;
    }
    qDebug()<<head.event<<" "<<head.size<<" "<<head.time;
    return true;
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
    tcp_isConnect = true;
    emit tcp_connected();
}
void TcpConnect::m_tcp_disconnected(){
    tcp_isConnect = false;
    emit tcp_disconnected();
}
//通知chatWidget连接失败
void TcpConnect::m_tcp_connect_error(QAbstractSocket::SocketError socketError){
    Q_UNUSED(socketError);
    tcp_isConnect =false;
    emit tcp_connect_error(m_socket->errorString());
}
