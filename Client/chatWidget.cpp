#include "chatWidget.h"
#include "ui_chatWidget.h"
#include <QMessageBox>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::chatWidget)
{
    ui->setupUi(this);
    m_tcpConn = new TcpConnect(this);

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
    if(!m_tcpConn->m_tcp_isConnect){
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
    ui->ip_lineEdit->setEnabled(false);
    ui->port_spinBox->setEnabled(false);
    ui->connect_pushButton->setText(u8"断开");
    m_tcpConn->m_tcp_isConnect = true;
}

//tcp断开连接
void ChatWidget::m_tcp_disconnected(){
    //tcp断开连接，改变按钮文字
    ui->ip_lineEdit->setEnabled(true);
    ui->port_spinBox->setEnabled(true);
    ui->connect_pushButton->setText(u8"连接");
    m_tcpConn->m_tcp_isConnect = false;
}

//tcp连接失败
void ChatWidget::m_tcp_connecte_error(const QString &error){
    QMessageBox::information(this,u8"提示",u8"ip或端口号错误，请检查!\n"+error,QMessageBox::Ok);
    m_tcpConn->m_tcp_isConnect = false;
}



