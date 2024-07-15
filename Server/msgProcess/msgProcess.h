#pragma once
#include "protocol.h"
#include "sys/epoll.h"
#include "msgUtils.h"
class MsgProcess{
public:
    MsgProcess(int epfd,int eventFd):m_epfd{epfd},m_eventFd{eventFd},m_isLogin{false}{};
    void run(struct epoll_event event);
private:
    void processRead();//处理读事件，从read buffer中读取数据
    void processWrite();//处理写事件，发送write buffer中数据
    bool stateTransition(const tcp_protocol::communication_events &head);//状态机进行状态装换

    bool func_login();//登陆事件
    bool func_login_success();//登陆成功
    bool func_login_failed();//登陆失败
    bool func_send_file();//前端发送文本数据，广播所有fd
private:
    bool m_isLogin;//是否登陆
    int m_eventFd;//socket对应的fd
    int m_epfd;//epoll fd，用于修改事件

    //TODO: 将协议头放在类里，是否有问题？
    tcp_protocol::communication_head m_head;//存储本次消息处理的协议头
};