#pragma once
#include "protocol.h"
#include "sys/epoll.h"
#include "msgUtils.h"
#include <map>
#include <list>
#include <mutex>
#include <memory>
class MsgProcess{
public:
    MsgProcess(int epfd,int eventFd,std::map<int,MsgProcess*> *fd2MsgProcess):
                        m_epfd{epfd},m_eventFd{eventFd},m_isLogin{false},m_fd2MsgProcess{fd2MsgProcess}{};
    void run(struct epoll_event &event);

public:
private:
    void processRead();//处理读事件，从read buffer中读取数据
    void processWrite();//处理写事件，发送write buffer中数据
    void processClose();//处理对端关闭
    bool stateTransition(tcp_protocol::communication_head &head);//状态机进行状态装换

    bool func_login(tcp_protocol::communication_head &head);//登陆事件
    bool func_login_success(tcp_protocol::communication_head &head);//登陆成功
    bool func_login_failed(tcp_protocol::communication_head &head);//登陆失败
    bool func_client_send_text(tcp_protocol::communication_head &head);//处理前端发送文本数据
    bool func_server_send_text();//广播前端发送的文本数据到所有fd

private:
    bool m_isLogin;//是否登陆
    int m_eventFd;//socket对应的fd
    int m_epfd;//epoll fd，用于修改事件

    std::list<std::shared_ptr<std::string>> m_text_buffer;//双向链表存储文本指针
    std::mutex m_text_buffer_mutex;//文本缓冲区锁

    std::map<int,MsgProcess*> *m_fd2MsgProcess;//socket fd和MsgProcess索引对 指针
    //TODO: 将协议头放在类里，是否有问题？
    // tcp_protocol::communication_head m_head;//存储本次消息处理的协议头
};