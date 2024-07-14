#include "chatWidget.h"
#include "ui_chatWidget.h"
#include <QMessageBox>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <cstring>
#include <cstdlib>
#include "protocol.h"
ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::chatWidget) ,m_tcpConn(new TcpConnect(this))
{
    ui->setupUi(this);

    connect(m_tcpConn,SIGNAL(tcp_connected()),this,SLOT(m_tcp_connected()));
    connect(m_tcpConn,SIGNAL(tcp_connect_error(QString)),this,SLOT(m_tcp_connecte_error(QString)));
    connect(m_tcpConn,SIGNAL(tcp_disconnected()),this,SLOT(m_tcp_disconnected()));
}

ChatWidget::~ChatWidget()
{
    delete ui;
}

//按下连接按钮
void ChatWidget::on_connect_pushButton_clicked()
{
    if(!m_tcpConn->tcp_isConnect){
        //点击连接按钮，获取 ip地址 和 端口号，和服务器建立tcp连接
        m_tcpConn->peer_ip = ui->ip_lineEdit->text();
        m_tcpConn->peer_port = ui->port_spinBox->value();
        //建立连接
        m_tcpConn->connectToServer();
    }else{
        //断开连接
        m_tcpConn->disconnectToServer();
    }
}

//tcp连接成功
void ChatWidget::m_tcp_connected(){
    //tcp连接成功，改变按钮文字
    m_tcpConn->tcp_isConnect = true;
    chatWidget_change();
}

//tcp断开连接
void ChatWidget::m_tcp_disconnected(){
    //tcp断开连接，改变按钮文字
    m_tcpConn->tcp_isConnect = false;
    chatWidget_change();
}

//tcp连接失败
void ChatWidget::m_tcp_connecte_error(const QString &error){
    QMessageBox::information(this,u8"提示",u8"ip或端口号错误，请检查!\n"+error,QMessageBox::Ok);
    m_tcpConn->tcp_isConnect = false;
    chatWidget_change();

}

//tcp连接状态改变时，修改界面
void ChatWidget::chatWidget_change(){
    //conn_flag true表示tcp成功连接
    bool conn_flag = m_tcpConn->tcp_isConnect;
    if(conn_flag){
        ui->connect_pushButton->setText(u8"断开");
        ui->login_title->setText(u8"请登陆");

    }else{
        ui->connect_pushButton->setText(u8"连接");
        ui->login_title->setText(u8"请连接服务器");
    }
    ui->ip_lineEdit->setEnabled(!conn_flag);
    ui->port_spinBox->setEnabled(!conn_flag);
    ui->login_pushButton->setEnabled(conn_flag);
    ui->register_pushButton->setEnabled(conn_flag);
    ui->username_lineEdit->setEnabled(conn_flag);
    ui->passwd_lineEdit->setEnabled(conn_flag);

    //login_flag true 表示登陆成功
    bool login_flag = m_userData->user_isLogin;
    ui->msg_input_textEdit->setEnabled(login_flag);
    ui->msg_output_listWidget->setEnabled(login_flag);
    ui->file_toolButton->setEnabled(login_flag);
    ui->img_toolButton->setEnabled(login_flag);

}

void ChatWidget::on_register_pushButton_clicked()
{
    //TODO:帐号注册页面
    QMessageBox::information(this,u8"提示",u8"暂不支持自己注册，请联系管理员注册",QMessageBox::Ok);
}


void ChatWidget::on_login_pushButton_clicked()
{
    //登陆
    //1.检查登陆状态
    if(!m_tcpConn->tcp_isConnect){
        QMessageBox::information(this,u8"提示",u8"服务器断开，请重新连接",QMessageBox::Ok);
    }
    //2. 获取帐号密码，md5加密，通过二进制数据发送
    QString username_text = ui->username_lineEdit->text();
    QString passswd_text = ui->passwd_lineEdit->text();
    //md5加密
    QString username_md5 = QCryptographicHash::hash(username_text.toUtf8(),QCryptographicHash::Md5).toHex();
    QString passwd_md5 = QCryptographicHash::hash(passswd_text.toUtf8(),QCryptographicHash::Md5).toHex();
    qDebug()<<"username_md5 "<<username_md5;
    //数据部分
//    char* data = (char*)malloc(buffer_sizes::LOGIN_BUFFER_SIZE*sizeof(char));
    //智能指针，防止内存泄漏
    std::unique_ptr<char[]> data(new char[buffer_sizes::LOGIN_BUFFER_SIZE]);
    memset(data.get(),0,buffer_sizes::LOGIN_BUFFER_SIZE*sizeof(char));
    QByteArray username_bytes = QByteArray::fromHex(username_md5.toUtf8());
    QByteArray passwd_bytes = QByteArray::fromHex(passwd_md5.toUtf8());
    qDebug()<<username_bytes.size()<<" "<<passwd_bytes.size();
    std::memcpy(data.get(), username_bytes.data(), username_bytes.size());
    std::memcpy(data.get()+username_bytes.size(), passwd_bytes.data(), passwd_bytes.size());
    //数据头
    tcp_protocol::communication_head head;
    head.event = static_cast<quint8>(tcp_protocol::communication_events::LOGIN);
    head.size = static_cast<quint64>(username_bytes.size() + passwd_bytes.size());
    head.time = static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
    if(!m_tcpConn->sendJsonData(head,data.get())){
        QMessageBox::critical(this,u8"错误",u8"登陆请求失败！",QMessageBox::Ok);
        return;
    }

//    QJsonObject login_data;
//    login_data["events"] = tcp_protocol::communication_events::LOGIN;
//    login_data["time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
//    login_data["data"] = data;
//    qDebug()<<sizeof(login_data.size());
//    qDebug()<<sizeof(login_data["events"]);
//    qDebug()<<sizeof(login_data["time"]);
//    qDebug()<<sizeof(login_data["data"]);

}

