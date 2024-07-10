#include "epollServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <unistd.h>
#include <cstring> // for strerror
#include <fcntl.h>
void set_nonblocking(int &fd);//设置文件为非阻塞
EpollServer::EpollServer(int port,int events_size){
    m_socket_port = port;//监听端口号
    m_events_size = events_size;//epoll_wait返回的最大事件数
}

EpollServer::~EpollServer(){
    close(m_listen_fd);
    close(m_epfd);
}

//初始化socket连接
void EpollServer::initSocket(){
    m_listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if(m_listen_fd == -1){
        std::cerr << "socket() error:  " << strerror(errno) << std::endl;
        return;
    }
    //端口重用
    int opt = 1;
    if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr<<"setsockopt() error: "<<strerror(errno)<<std::endl;
        close(m_listen_fd);
        return;

    }
    //非阻塞
    set_nonblocking(m_listen_fd);

    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(this->m_socket_port);
    if(-1 ==bind(m_listen_fd,(const sockaddr*)&server_addr,sizeof(server_addr))){
        std::cerr<<"bind() error: "<<strerror(errno)<<std::endl;
        close(m_listen_fd);
        return;
    }
    
    if(-1 == listen(m_listen_fd,5)){
        std::cerr<<"listen() error: "<<strerror(errno)<<std::endl;
        close(m_listen_fd);
        return;
    }

}

// 设置文件描述符为非阻塞模式
void set_nonblocking(int &fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr<<"fcntl() error: "<<strerror(errno)<<std::endl;
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr<<"fcntl() error: "<<strerror(errno)<<std::endl;
        return;
    }
}

void EpollServer::run()
{
    //线程池监听读写列表，并处理
    m_thread_pool = new threadPool<rw_event_struct>;
    //主线程接收连接，读写事件放入列表中
    epollReactor();
}

void EpollServer::epollReactor(){
    initSocket();
    m_epfd = epoll_create(1);
    //epoll_wait 接收返回值数组
    struct epoll_event events[m_events_size];
    //epoll 监听socket
    struct epoll_event event;
    event.data.fd = m_listen_fd;
    event.events = EPOLLIN;//边沿模式
    epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_listen_fd,&event);
    while(1){
        int nready = epoll_wait(m_epfd,events,m_events_size,0);
        for(int i=0;i<nready;i++){
            int event_fd = events[i].data.fd;
            if(event_fd == m_listen_fd){
                //有新连接
                std::cout<<"listen fd: "<<m_listen_fd<<std::endl;
                sockaddr_in client_addr;
                ssize_t client_size;
                int client_fd = accept(m_listen_fd,(sockaddr*)&client_addr,(socklen_t*)&client_size);
                if(client_fd == -1){
                    std::cerr << "accept() error:  " << strerror(errno) << std::endl;
                    return;
                }
                event.data.fd = client_fd;
                event.events = EPOLLIN|EPOLLET;//边沿模式
                epoll_ctl(m_epfd,EPOLL_CTL_ADD,client_fd,&event);
                std::cout<<"connect success! fd = "<<client_fd<<std::endl;
            }else{
                //处理读事件和写事件
                if(events[i].events == EPOLLIN){
                    //TODO：开新线程，处理事件，队列要加锁，影响性能？
                    auto task = std::make_shared<rw_event_struct>();
                    task->arg = events[i];
                    task->func = std::bind(&EpollServer::handler_read, this, std::placeholders::_1);
                    m_thread_pool->append(task);
                }else if(events[i].events == EPOLLOUT){
                   auto task = std::make_shared<rw_event_struct>();
                    task->arg = events[i];
                    task->func = std::bind(&EpollServer::handler_write, this, std::placeholders::_1);
                    m_thread_pool->append(task);
                }else{
                    //TODO：其它事件处理
                }
            }
        }
    }
}

//处理读事件
void EpollServer::handler_read(struct epoll_event event){
    std::cout<<"this is EPOLLIN"<<std::endl;
    event.events = EPOLLOUT|EPOLLET;//边沿模式
    epoll_ctl(m_epfd,EPOLL_CTL_MOD,event.data.fd,&event);
}
//处理写事件
void EpollServer::handler_write(struct epoll_event event){
    std::cout<<"this is EPOLLOUT"<<std::endl;

}
