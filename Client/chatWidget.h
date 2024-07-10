#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>

#include "tcpConnect.h"

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
    Ui::chatWidget *ui;
    TcpConnect* m_tcpConn;//tcp连接对象

private slots:
    void on_connect_pushButton_clicked();//连接/断开按钮
    void m_tcp_connected();//tcp连接成功，改变按钮
    void m_tcp_disconnected();//tcp断开连接，改变按钮
    void m_tcp_connecte_error(const QString &error);//tcp连接错误
};
#endif // CHATWIDGET_H
