#include "tcpConnect.h"
#include <QHostAddress>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
int fromEventGetBufferSize(tcp_protocol::communication_events event);
TcpConnect::TcpConnect(QObject *parent)
    : QObject(parent),
    m_socket(new QTcpSocket(this))
{
    tcp_isConnect = false;
    connect(m_socket,SIGNAL(connected()),this,SLOT(m_tcp_connected()));
    connect(m_socket,SIGNAL(errorOccurred(QAbstractSocket::SocketError)),this,
            SLOT(m_tcp_connect_error(QAbstractSocket::SocketError)));
//    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpConnect::m_tcp_connect_error);
    connect(m_socket,SIGNAL(disconnected()),this,SLOT(m_tcp_disconnected()));
    connect(m_socket,SIGNAL(readyRead()),this,SLOT(m_tcp_handler_read()));
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
    send_buffer.append(data,head.info_size + head.size);
    //tcp socket发送
    int ret = m_socket->write(send_buffer);
    m_socket->flush();
    if(-1 == ret){
        qDebug()<<"tcp send data error: "<<m_socket->error();
        return false;
    }
    qDebug()<<head.event<<" "<<head.size<<" "<<head.time<<" "<<head.info_size;
    qDebug()<<"send length: "<<head.size + head.info_size;
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
    tcp_isConnect = false;
    qDebug()<<"link error::::::::::::::::::::::::"<<socketError;
    emit tcp_connect_error(m_socket->errorString());
}

//tcp连接接收消息 readReady 槽函数
void TcpConnect::m_tcp_handler_read(){
        if(!this->tcp_isConnect){
            qDebug()<<"tcp disconnect!";
        }
        //1.读取head数据
        //静态flag，控制读头还是数据，否则会数据丢失
        static bool isReadingHead = true;
        static bool isReadInfoData = false;
        tcp_protocol::communication_head head;
        if (isReadingHead) {
            if (m_socket->bytesAvailable() < static_cast<int>(sizeof(head))) {
                return;
            }
            m_socket->read(reinterpret_cast<char*>(&head), sizeof(head));
            if(head.info_size >0 ) isReadInfoData = true;
            isReadingHead = false;
        }
        //2. 根据事件，发送函数，处理 res_data (类型转换)
        switch(head.event){
            case tcp_protocol::LOGIN_SUCCESS:
            {
                emit tcp_login_response(head);
                break;
            }
            case tcp_protocol::LOGIN_FAILED:
            {
                emit tcp_login_response(head);
                break;
            }
            case tcp_protocol::SERVER_SEND_TEXT:
            {
                qDebug()<<"recv text!";
                //服务端发送文本数据
                //1.读取data数据
                QByteArray res_data;
                res_data.clear();
                res_data.resize(head.size);
                if (!isReadingHead) {
                    if (m_socket->bytesAvailable() < head.size) {
                        qDebug()<<"读文本数据失败！";
                        return;
                    }
                    m_socket->read(res_data.data(), head.size);
                    // 准备读取下一条消息
                }
                //2. 发送信号，处理事件
                emit tcp_recv_text(QString::fromUtf8(res_data));
                break;
            }
            case tcp_protocol::SERVER_SEND_FILE_LINK:
            {
                //服务端发送下载文件链接
                //head.size存储 文件链接数据长度
                QByteArray res_data;
                res_data.clear();
                res_data.resize(head.size);
                if(!isReadingHead){
                    if(m_socket->bytesAvailable()<head.size){
                        return;
                    }
                    m_socket->read(res_data.data(),head.size);
                }
                emit tcp_recv_file_link(QString::fromUtf8(res_data));
                break;
            }
            case tcp_protocol::SERVER_SEND_FILE:
            {
                //服务端发送文件
                QByteArray res_data;
                res_data.clear();
                res_data.resize(head.size);
                QString filename;
                if(!isReadingHead){
                    //1. 先读取文件名
                    if(isReadInfoData){
                        if(m_socket->bytesAvailable()<head.info_size){
                            return;
                        }
                        m_socket->read(res_data.data(),head.info_size);
                        filename = QString::fromUtf8(res_data,head.info_size);
                        isReadInfoData = false;
                    }
                    //2. 读取文件
                    res_data.clear();
                    char buffer[buffer_sizes::SERVER_SEND_FILE_BUFFER_SIZE];
                    quint64 remind_data = head.size;
                    while(remind_data>0){
                        int tmp = remind_data<buffer_sizes::SERVER_SEND_FILE_BUFFER_SIZE?remind_data:buffer_sizes::SERVER_SEND_FILE_BUFFER_SIZE;
                        while(m_socket->bytesAvailable()<tmp){
                            if (!m_socket->waitForReadyRead(3000)){
                                qDebug()<<"wait";
                            }
                        }
                        qDebug() << "Attempting to read" << tmp << "bytes, bytesAvailable:" << m_socket->bytesAvailable();
                        memset(buffer,0,sizeof(buffer));
                        int ret = m_socket->read(buffer,tmp);
                        if (ret < 0) {
                            qDebug() << "Error while reading data.";
                            return;
                        }
                        remind_data -= ret;
                        res_data.append(buffer,ret);
                    }

                }
                emit tcp_recv_file(filename,res_data);
                break;
            }
            default:{
                qDebug()<<"error: "<<head.event<<" "<<head.size<<" "<<head.time<<" "<<head.info_size;
                break;
            }
        }

        isReadingHead = true;

}

