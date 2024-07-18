#include "chatWidget.h"
#include "ui_chatWidget.h"
#include <QMessageBox>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <cstring>
#include <cstdlib>
#include <QKeyEvent>
#include "protocol.h"
#include <QFileDialog>
ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::chatWidget) ,m_tcpConn(new TcpConnect(this))
{
    ui->setupUi(this);

    connect(m_tcpConn,SIGNAL(tcp_connected()),this,SLOT(m_tcp_connected()));
    connect(m_tcpConn,SIGNAL(tcp_connect_error(QString)),this,SLOT(m_tcp_connect_error(QString)));
    connect(m_tcpConn,SIGNAL(tcp_disconnected()),this,SLOT(m_tcp_disconnected()));
    connect(this,SIGNAL(sendTextMsg(QString)),this,SLOT(m_send_text_msg(QString)));
    connect(m_tcpConn,SIGNAL(tcp_login_response(tcp_protocol::communication_head)),this,SLOT(m_login_response(tcp_protocol::communication_head)));
    connect(m_tcpConn,SIGNAL(tcp_recv_text(QString)),this,SLOT(m_recv_text(QString)));
    connect(m_tcpConn,SIGNAL(tcp_recv_file_link(QString)),this,SLOT(m_recv_file_link(QString)));
    //输入文本框安装事件过滤器
    ui->msg_input_textEdit->installEventFilter(this);
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
void ChatWidget::m_tcp_connect_error(const QString &error){
    QMessageBox::information(this,u8"提示",u8"ip或端口号错误，请检查!\n"+error,QMessageBox::Ok);
    m_tcpConn->tcp_isConnect = false;
    chatWidget_change();

}

//tcp连接状态改变时，修改界面
void ChatWidget::chatWidget_change(){
    //conn_flag true表示tcp成功连接
    bool conn_flag = m_tcpConn->tcp_isConnect;
    //tcp连接模块
    ui->ip_lineEdit->setEnabled(!conn_flag);
    ui->port_spinBox->setEnabled(!conn_flag);

    //login_flag true 表示登陆成功 且 tcp连接成功
    bool login_flag = m_userData->isLogin;
    //左侧消息接收发送模块
    ui->msg_input_textEdit->setEnabled(login_flag);
    ui->msg_output_listWidget->setEnabled(login_flag);
    ui->file_toolButton->setEnabled(login_flag);
    //登陆模块
    ui->login_pushButton->setEnabled(conn_flag && !login_flag);
    ui->register_pushButton->setEnabled(conn_flag && !login_flag);
    ui->username_lineEdit->setEnabled(conn_flag && !login_flag);
    ui->passwd_lineEdit->setEnabled(conn_flag && !login_flag);
    //渲染文本
    if(conn_flag){
        //tcp连接成功
        ui->connect_pushButton->setText(u8"断开");
        if(login_flag){
            //tcp连接成功 && 登陆
            ui->login_title->setText(u8"登陆成功");
        }else{
            //tcp连接成功 && 未登陆
            ui->login_title->setText(u8"请登陆");
        }
    }else{
        //tcp未连接
        ui->connect_pushButton->setText(u8"连接");
        ui->login_title->setText(u8"请连接服务器");
    }
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
    //数据部分
    //智能指针，防止内存泄漏
    std::unique_ptr<char[]> data(new char[buffer_sizes::LOGIN_BUFFER_SIZE]);
    memset(data.get(),0,buffer_sizes::LOGIN_BUFFER_SIZE*sizeof(char));
    QByteArray username_bytes = QByteArray::fromHex(username_md5.toUtf8());
    QByteArray passwd_bytes = QByteArray::fromHex(passwd_md5.toUtf8());
    std::memcpy(data.get(), username_bytes.data(), username_bytes.size());
    std::memcpy(data.get()+username_bytes.size(), passwd_bytes.data(), passwd_bytes.size());
    //数据头
    tcp_protocol::communication_head head;
    head.event = static_cast<quint8>(tcp_protocol::communication_events::LOGIN);
    head.size = static_cast<quint64>(username_bytes.size() + passwd_bytes.size());
    head.time = static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
    if(!m_tcpConn->sendData(head,data.get())){
        QMessageBox::critical(this,u8"错误",u8"登陆请求失败！",QMessageBox::Ok);
        return;
    }

}
//接收登陆请求回应
void ChatWidget::m_login_response(tcp_protocol::communication_head head){
    //解析从服务器收到的head数据
    if(head.event == tcp_protocol::communication_events::LOGIN_FAILED){
        QMessageBox::information(this,u8"提示",u8"帐号或密码错误，登陆失败！",QMessageBox::Ok);
        return;
    }
    m_userData->isLogin = true;
    chatWidget_change();
}

//事件过滤器
bool ChatWidget::eventFilter(QObject *watched, QEvent *event){
    if(watched == ui->msg_input_textEdit && event->type() == QEvent::KeyPress){
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
//        qDebug() << "Key pressed: " << keyEvent->key();
        //回车键事件
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // 检查是否按下了 Shift 键
            if (!(keyEvent->modifiers() & Qt::ShiftModifier)) {
                // 没有按 Shift+回车键，发送消息，否则默认换行
                key_return_mInputTextEdit();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched,event);
}

//输入文本框按下回车键事件
void ChatWidget::key_return_mInputTextEdit(){
    //1. 获取数据
    QString input_msg = ui->msg_input_textEdit->toPlainText();
    if(input_msg.trimmed().isEmpty()) return;
//    QString input_msg = u8"123";
    //TODO: 文件/图片消息，如何链接到input_textEdit中？
    //2. 发送到服务器
    //数据部分
    //智能指针，防止内存泄漏
    auto send_data_type = tcp_protocol::communication_events::CLIENT_SEND_TEXT;
    int send_buffer_size =buffer_sizes::CLIENT_SEND_TEXT_BUFFER_SIZE;
    //TODO：循环发送消息
    std::unique_ptr<char[]> data(new char[send_buffer_size]);
    memset(data.get(),0,send_buffer_size*sizeof(char));
    QByteArray send_data = input_msg.toUtf8();
    std::memcpy(data.get(), send_data.data(), send_data.size());
    //数据头
    tcp_protocol::communication_head head;
    head.event = static_cast<quint8>(send_data_type);
    head.size = static_cast<quint64>(send_data.size());
    head.time = static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
    if(!m_tcpConn->sendData(head,data.get())){
        QMessageBox::critical(this,u8"错误",u8"发送数据失败！",QMessageBox::Ok);
        return;
    }
    //3. 通过信号 渲染到 msg_output_listWidget
    emit sendTextMsg(input_msg);
    //4. 清空输入框
    ui->msg_input_textEdit->clear();
    ui->msg_input_textEdit->moveCursor(QTextCursor::Start);
}

//发送数据，渲染 list widget
void ChatWidget::m_send_text_msg(const QString input_msg){
    QLabel *label = new QLabel(input_msg, this);
    //启用标签的自动换行功能
    label->setWordWrap(true);

    // 创建QWidget作为容器
    QWidget *widgetContainer = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widgetContainer);
    layout->addStretch();  // 添加一个伸展项目，使文本靠右对齐
    layout->addWidget(label);
    layout->setContentsMargins(0, 10, 20, 10);  // 去除边距
    layout->setSpacing(0);  // 去除间距

    QListWidgetItem *item = new QListWidgetItem(ui->msg_output_listWidget);
    item->setSizeHint(widgetContainer->sizeHint());

    ui->msg_output_listWidget->addItem(item);
    ui->msg_output_listWidget->setItemWidget(item, widgetContainer);
    ui->msg_input_textEdit->clear();

}

//接收到文本消息，渲染到 list widget
void ChatWidget::m_recv_text(const QString text){
    QLabel *label = new QLabel(text,this);
    label->setWordWrap(true);
    QWidget *widgetContainer = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widgetContainer);
    layout->addWidget(label);
    layout->addStretch();
    layout->setContentsMargins(20,10,0,10);
    layout->setSpacing(0);

    QListWidgetItem *item = new QListWidgetItem(ui->msg_output_listWidget);
    item->setSizeHint(widgetContainer->sizeHint());
    ui->msg_output_listWidget->addItem(item);
    ui->msg_output_listWidget->setItemWidget(item, widgetContainer);
}

//点击发送文件
void ChatWidget::on_file_toolButton_clicked()
{
    //1. 获取文件名字
    QString filepath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*)"));
    if(filepath.isEmpty()) return;
    qDebug()<<"filepath: "<<filepath;
    //2. 打开文件
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("错误"), tr("打开文件失败"));
        return;
    }
    QByteArray send_data = file.readAll();
    file.close();
    //3. 发送文件
    // 数据头
    tcp_protocol::communication_head head;
    head.event = tcp_protocol::communication_events::CLIENT_UPLOAD_FILE;
    head.size = send_data.size();
    head.time = static_cast<quint64>(QDateTime::currentSecsSinceEpoch());
    //文件长度
    QFileInfo fileInfo(filepath);
    QString filename = fileInfo.fileName();
    qDebug()<<"filename: "<<filename;
    QByteArray fileByte = filename.toUtf8();
    head.info_size = fileByte.size();

    std::unique_ptr<char[]> data(new char[head.size+head.info_size]);
    memset(data.get(),0,(head.size + head.info_size)*sizeof(char));
    std::memcpy(data.get(),fileByte.data(),head.info_size);
    std::memcpy(data.get()+head.info_size, send_data.data(), send_data.size());
    if(!m_tcpConn->sendData(head,data.get())){
        QMessageBox::critical(this,u8"错误",u8"发送数据失败！",QMessageBox::Ok);
        return;
    }
    //4. 发送信号，渲染界面
}

//收到下载链接，渲染到聊天界面中
void ChatWidget::m_recv_file_link(const QString text){
    qDebug()<<"recv file link: "<<text;
}


