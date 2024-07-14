#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>

#include "tcpConnect.h"
#include "userData.h"
QT_BEGIN_NAMESPACE
namespace Ui { class chatWidget; }
QT_END_NAMESPACE

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();
private:
    void chatWidget_change();//改变页面状态
private:
    Ui::chatWidget *ui;
    TcpConnect* m_tcpConn;//tcp连接对象
    UserData* m_userData;//用户数据对象
private slots:
    void on_connect_pushButton_clicked();//连接/断开按钮
    void m_tcp_connected();//tcp连接成功，改变按钮
    void m_tcp_disconnected();//tcp断开连接，改变按钮
    void m_tcp_connecte_error(const QString &error);//tcp连接错误
    void on_register_pushButton_clicked();
    void on_login_pushButton_clicked();
};
#endif // CHATWIDGET_H
