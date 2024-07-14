#pragma once
#include "protocol.h"
#include "sys/epoll.h"
#include "msgUtils.h"
class MsgProcess{
public:
    MsgProcess(int epfd,int eventFd):m_epfd{epfd},m_eventFd{eventFd}{};
    void run(struct epoll_event event);
private:
    void processRead();//处理读事件，从read buffer中读取数据
    void processWrite();//处理写事件，发送write buffer中数据
    bool stateTransition(const tcp_protocol::communication_events &head);//状态机进行状态装换

    bool func_login();//登陆事件
    bool func_login_success();//登陆成功
    bool func_login_failed();//登陆失败
private:
    bool m_isLogin;//
    bool m_isRoot;//
    int m_eventFd;//socket对应的fd
    int m_epfd;//epoll fd，用于修改事件
};