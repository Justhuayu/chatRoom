#ifndef TCPCONNECT_H
#define TCPCONNECT_H
#include "protocol.h"
#include <QObject>
#include <QTcpSocket>
//tcp连接，用于收发数据
class TcpConnect : public QObject
{
    Q_OBJECT
public:
    explicit TcpConnect(QObject *parent = nullptr);

    void connectToServer();//连接tcp
    void disconnectToServer();//断开tcp连接
    bool sendData(const tcp_protocol::communication_head &head,const char* data);//发送二进制数据到后端
//    tcp_protocol::communication_head recvData();//从后端接收二进制数据
public:
    QString peer_ip;//ip
    int16_t peer_port;//端口
    bool tcp_isConnect;//tcp连接状态
signals:
    void tcp_connected();//tcp连接成功后，通知widget
    void tcp_disconnected();//tco断开连接后，通知widget
    void tcp_connect_error(QString error);//tcp连接错误
    void tcp_login_response(tcp_protocol::communication_head head);
    void tcp_recv_text(QString text);//接收到文本
    void tcp_recv_file_link(QString text);//接收文件下载地址
    void tcp_recv_file(QString filename,QByteArray data);//接收文件

private:
    QTcpSocket *m_socket;
private slots:
    void m_tcp_connected();//socket连接成功信号对应槽函数，通知chatWidget连接成功（emit tcp_connected）
    void m_tcp_connect_error(QAbstractSocket::SocketError socketError);//连接错误
    void m_tcp_disconnected();//通知chatWidget断开连接
    void m_tcp_handler_read();//解析tcp连接接收消息
};

#endif // TCPCONNECT_H
