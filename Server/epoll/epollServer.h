#pragma once
#include <iostream>
#include <list>
#include "threadPool.h"
#include <functional>
#include <sys/epoll.h>
#include <map>
#include "msgProcess.h"

//读写工作队列结构体，用于线程池执行
struct rw_event_struct{
    //存放epoll_event 和对应的执行函数
    struct epoll_event arg;//参数
    std::function<void(struct epoll_event)> func;//函数

};

class EpollServer{
public:
    EpollServer(int port,int events_size);
    ~EpollServer();
    void run();
public:
    
private:
    void initSocket();//socket 、bind 、listen
    void epollReactor();//epoll reactor模式

    void handler_event(struct epoll_event event);//处理事件
private:
    int m_socket_port;//监听端口号
    int m_events_size;//epoll_wait返回的最大数量
    
    int m_listen_fd;//socket监听句柄
    int m_epfd;//epoll句柄
    std::map<int,MsgProcess*> m_fd2MsgProcess;//socket fd和MsgProcess索引对
    // std::list<rw_event_struct> rw_event_list;//读写事件列表

    ThreadPool<rw_event_struct> *m_thread_pool;//线程池，处理读写事件列表
};