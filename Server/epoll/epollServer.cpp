#include "epollServer.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <unistd.h>
#include <cstring> // for strerror

std::list<int> EpollServer::read_event_list;
std::list<int> EpollServer::write_event_list;

EpollServer::EpollServer(int port,int events_size){
    socket_port = port;//监听端口号
    events_size = events_size;//epoll_wait返回的最大事件数
}

EpollServer::~EpollServer(){
    close(listen_fd);
    close(epfd);
}

void EpollServer::epollReactor(){
    initSocket();
    epfd = epoll_create(1);
    //epoll_wait 接收返回值数组
    struct epoll_event events[events_size];
    //epoll 监听socket
    struct epoll_event event;
    event.data.fd = listen_fd;
    event.events = EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,listen_fd,&event);
    while(1){
        int nready = epoll_wait(epfd,events,events_size,0);
        for(int i=0;i<nready;i++){
            int event_fd = events[i].data.fd;
            if(event_fd == listen_fd){
                //有新连接
                sockaddr_in client_addr;
                ssize_t client_size;
                int client_fd = accept(listen_fd,(sockaddr*)&client_addr,(socklen_t*)&client_size);
                if(client_fd == -1){
                    std::cerr << "accept() error:  " << strerror(errno) << std::endl;
                    return;
                }
                event.data.fd = client_fd;
                event.events = EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,client_fd,&event);
            }else{
                //处理读事件和写事件
                if(events[i].events == EPOLLIN){
                    read_event_list.push_back(event_fd);
                }else if(events[i].events == EPOLLOUT){
                    write_event_list.push_back(event_fd);
                }else{
                    //TODO：其它事件处理
                    
                }
            }
        }
    }
}

//初始化socket连接
void EpollServer::initSocket(){
    listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd == -1){
        std::cerr << "socket() error:  " << strerror(errno) << std::endl;
        return;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(this->socket_port);
    if(-1 ==bind(listen_fd,(const sockaddr*)&server_addr,sizeof(server_addr))){
        std::cerr<<"bind() error: "<<strerror(errno)<<std::endl;
        return;
    }
    if(-1 == listen(listen_fd,5)){
        std::cerr<<"listen() error: "<<strerror(errno)<<std::endl;
        return;
    }
    
}