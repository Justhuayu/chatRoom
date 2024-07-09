#pragma once
#include <iostream>
#include <list>
class EpollServer{
public:
    EpollServer(int port,int events_size);
    ~EpollServer();
public:
    static std::list<int> read_event_list;//读事件列表
    static std::list<int> write_event_list;//写事件列表
private:
    void initSocket();//socket 、bind 、listen
    void epollReactor();//epoll reactor模式

private:
    int socket_port;//监听端口号
    int events_size;//epoll_wait返回的最大数量
    
    int listen_fd;//socket监听句柄
    int epfd;//epoll句柄


};