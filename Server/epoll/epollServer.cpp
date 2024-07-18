#include "epollServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <unistd.h>
#include <cstring> // for strerror
#include <fcntl.h>
#include <memory>
#include <arpa/inet.h>

void set_nonblocking(int &fd);//设置文件为非阻塞
EpollServer::EpollServer(int port,int events_size){
    m_socket_port = port;//监听端口号
    m_events_size = events_size;//epoll_wait返回的最大事件数
}

EpollServer::~EpollServer(){
    close(m_listen_fd);
    close(m_epfd);
    MsgProcess *msg=nullptr;
    for(auto it=m_fd2MsgProcess.begin();it!=m_fd2MsgProcess.end();it++){
        msg = it->second;
        delete msg;
        msg = nullptr;
        close(it->first);
    }
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
    m_thread_pool = new ThreadPool<rw_event_struct>;
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
                sockaddr_in client_addr;
                ssize_t client_size = sizeof(client_addr);
                int client_fd = accept(m_listen_fd,(sockaddr*)&client_addr,(socklen_t*)&client_size);
                if(client_fd == -1){
                    std::cerr << "accept() error:  " << strerror(errno) << std::endl;
                    return;
                }
                std::cout<<"listen fd: "<<m_listen_fd<<"client fd: "<<client_fd<<std::endl;

                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                std::cout << "Client connected from IP: " << client_ip << ", port: " << ntohs(client_addr.sin_port) << std::endl;
                set_nonblocking(client_fd);//非阻塞
                event.data.fd = client_fd;
                event.events = EPOLLIN|EPOLLET;//边沿模式
                epoll_ctl(m_epfd,EPOLL_CTL_ADD,client_fd,&event);
                //新连接对应一个MsgProcess类，用于存储登陆状态、权限等信息
                //TODO: MsgProcess参数 m_fd2MsgProcess用引用，为什么不同连接会有多个地址？
                MsgProcess* msg = new MsgProcess(m_epfd,client_fd,&m_fd2MsgProcess);
                m_fd2MsgProcess.insert(std::pair<int,MsgProcess*>(client_fd,msg));
                std::cout<<"connect success! fd = "<<client_fd<<std::endl;
            }else{
                //处理读事件和写事件
                // if(events[i].events != EPOLLIN && events[i].events != EPOLLOUT){
                //     //TODO：其它事件处理
                //     //TODO：关闭socket，根据socket fd 释放MsgProcess
                    
                // }
                //TODO：开新线程，处理事件，队列要加锁，影响性能？
                auto task = std::make_shared<rw_event_struct>();
                task->arg = events[i];
                task->func = std::bind(&EpollServer::handler_event, this, std::placeholders::_1);
                m_thread_pool->append(task);
            }
        }
    }
}

//处理事件
void EpollServer::handler_event(struct epoll_event event){
    // std::cout<<"this is EPOLLIN"<<std::endl;
    // event.events = EPOLLOUT|EPOLLET;//边沿模式
    // epoll_ctl(m_epfd,EPOLL_CTL_MOD,event.data.fd,&event);
    
    //调试多线程是否成功
    pthread_t thread_id = pthread_self();
    std::cout<<"当前执行线程： "<<thread_id<<" 处理fd："<<event.data.fd<<std::endl;

    for(auto item = m_fd2MsgProcess.begin();item!=m_fd2MsgProcess.end();item++){
        std::cout<<"fd: "<<item->first<<" ";
    }
    std::cout<<std::endl;
    auto it = m_fd2MsgProcess.find(event.data.fd);
    if(it == m_fd2MsgProcess.end()){
        std::cout<<"fd"<<event.data.fd<< "未连接或已断开连接"<<std::endl;
        return;
    }

    MsgProcess* msg = it->second;
    msg->run(event);
}
