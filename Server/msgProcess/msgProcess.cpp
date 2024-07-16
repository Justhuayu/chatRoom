#include "msgProcess.h"
#include "protocol.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <algorithm>
void MsgProcess::run(struct epoll_event &event){
    // std::cout<<"fd2MsgProcess"<<m_fd2MsgProcess<<std::endl;
    if(this->m_eventFd != event.data.fd){
        std::cout<<"[ERROR]MsgProcess 中 fd "<<m_eventFd<<"和待处理事件 fd "<<event.data.fd<<"不相等！"<<std::endl;
        return;
    }
    if(event.events == EPOLLIN){
        //读事件
        processRead();
    }else if(event.events == EPOLLOUT){
        //写事件
        processWrite();
    }else if(event.events == EPOLLRDHUP){
        //对端关闭，清理资源
        processClose();
    }else{
        //TODO：处理其它事件
    }
}

//处理读事件
void MsgProcess::processRead(){
    //1. 从read buffer中读取头
    tcp_protocol::communication_head head;
    memset(&head,0,sizeof(head));
    // int ret = read(this->m_eventFd,&m_head,sizeof(m_head));
    ssize_t ret;
    ret =read(this->m_eventFd,&head,sizeof(head));
    std::cout<<m_eventFd<<" "<<ret<<" "<<static_cast<unsigned int>(head.event)<<" "<<head.size<<" "<<head.time<<std::endl;
    if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) return;
    //2. 根据事件类型，读取数据
    stateTransition(head);
}

//处理写事件
void MsgProcess::processWrite(){
    //发送缓冲区消息
    //发送文本
    tcp_protocol::communication_head head;
    memset(&head,0,sizeof(head));
    std::cout<<"send fd: "<<m_eventFd<<std::endl;
    while(!m_text_buffer.empty()){
        head.event = tcp_protocol::communication_events::SERVER_SEND_TEXT;
        head.time = MsgUtils::getCurrentTimeInSeconds(); 
        std::string* send_data;
        {
            std::lock_guard<std::mutex> lock(m_text_buffer_mutex);
            send_data = m_text_buffer.front().get();
            m_text_buffer.pop_front();
        }
        head.size = send_data->size();
        //1. 先发送头
        int remind_data = sizeof(head);
        int ret = write(m_eventFd,&head,sizeof(head));
        if(!MsgUtils::writeError(ret,m_epfd,m_eventFd)) return;
        //2. 发送写数据
        //循环写事件
        remind_data = send_data->size();
        std::cout<<"发送数据： "<<*send_data<<std::endl;
        const char* data_ptr = send_data->c_str(); // 获取指向数据的指针
        std::cout<<"head.size :"<<head.size<<" data_ptr->size: "<< std::strlen(data_ptr)<<std::endl;

        while(remind_data>0){
            ret = write(m_eventFd,data_ptr + send_data->size() - remind_data,remind_data);
            if(!MsgUtils::writeError(ret,this->m_epfd,this->m_eventFd)) return;
            remind_data -= ret;
        }
    }
    //发送图片
    //发送文件
    //3. 写完数据后改成读事件
    //TODO: 不修改为读事件，会阻塞?
    MsgUtils::epoll_mod_event_read(m_epfd,m_eventFd);

}

//处理对端关闭
void MsgProcess::processClose(){
    //1. 从fd2MsgProcess中删除
    auto it = m_fd2MsgProcess->find(m_eventFd);
    MsgProcess* msg = nullptr;
    if(it != m_fd2MsgProcess->end()){
        msg = it->second;
        m_fd2MsgProcess->erase(it);
    }
    //2.清理资源
    if(!msg){
        delete msg;
    }
    epoll_ctl(m_epfd,EPOLL_CTL_DEL,m_eventFd,nullptr);
    close(m_eventFd);
    std::cout<<"断开连接"<<std::endl;
}

//消息处理入口函数，状态机调用其它状态
bool MsgProcess::stateTransition(tcp_protocol::communication_head &head){
    switch (head.event)
    {
    case tcp_protocol::LOGIN:
        //登陆
        func_login(head);
        break;
    case tcp_protocol::LOGIN_SUCCESS:
        //登陆成功
        func_login_success(head);
        break;
    case tcp_protocol::LOGIN_FAILED:
        func_login_failed(head);
        //登陆失败
        break;
    case tcp_protocol::LOGIN_EXIT:
        //退出登陆
        break;
    case tcp_protocol::CLIENT_SEND_TEXT:
        //发送文本数据请求
        func_client_send_text(head);
        break;
    case tcp_protocol::SERVER_SEND_TEXT:
        func_server_send_text();
        break;
    default:
        std::cout<<"status 错误"<<std::endl;
        return false;
        break;
    }
    return true;
}
//处理客户端发送到文本消息
bool MsgProcess::func_client_send_text(tcp_protocol::communication_head &head){
    //1. 读数据
    // if(!m_isLogin) return false;
    char buffer[buffer_sizes::CLIENT_SEND_TEXT_BUFFER_SIZE];
    int remind_data = head.size;
    std::shared_ptr<std::string> recv_text = std::make_shared<std::string>();
    recv_text->reserve(head.size);
    //边沿模式循环读数据
    while(remind_data>0){
        int tmp = std::min(head.size,sizeof(buffer));
        int ret = read(this->m_eventFd,buffer,tmp);
        if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) return false;
        remind_data -= ret;
        recv_text->append(buffer,ret);
    }
    // recv_text->push_back('\0');//字符串结束符号
    //2. 存储数据
    //TODO：数据存储到数据库
    {
        //加锁修改缓冲区
        std::lock_guard<std::mutex> lock(m_text_buffer_mutex);
        std::cout<<"recv_text: "<<*recv_text<<std::endl;
        
        m_text_buffer.push_back(recv_text);
    }
    //3. 发送数据
    //广播发送数据
    head.size = 0;
    head.event = tcp_protocol::SERVER_SEND_TEXT;
    stateTransition(head);
    return true;
}

//广播客户端发送的文本消息
bool MsgProcess::func_server_send_text(){
    int fd;
    MsgProcess *msg;
    std::shared_ptr<std::string> send_data;
    {
        std::lock_guard<std::mutex> lock(m_text_buffer_mutex);
        send_data = m_text_buffer.front();
        m_text_buffer.pop_front();
    }
    for(auto it = m_fd2MsgProcess->begin();it != m_fd2MsgProcess->end();it++){
        fd = it->first;
        msg = it->second;
        //广播，不处理自己客户端fd
        if(msg->m_eventFd == this->m_eventFd) continue;
        //1. 修改所有fd状态为写
        MsgUtils::epoll_mod_event_write(msg->m_epfd,fd);
        //2. 所有fd对应的MsgProcess的文本缓冲区，添加要发送的数据
        {
            std::lock_guard<std::mutex> lock(msg->m_text_buffer_mutex);
            msg->m_text_buffer.push_back(send_data);
        }
    }
    return true;
}
//登陆成功
bool MsgProcess::func_login_success(tcp_protocol::communication_head &head){   
    m_isLogin = true;
    //发送数据
    //1.准备数据头
    head.event = tcp_protocol::communication_events::LOGIN_SUCCESS;
    //TODO: 成功发送历史聊天记录里
    head.size = 0;
    head.time = MsgUtils::getCurrentTimeInSeconds();
    // std::cout<<m_head.time<<std::endl;
    //2.发送数据
    int remind_data = sizeof(head);
    int ret;
    while(remind_data>0){
        ret = write(m_eventFd,&head+sizeof(head) - remind_data,remind_data);
        if(!MsgUtils::writeError(ret,this->m_epfd,this->m_eventFd)) return false;
        remind_data-=ret;
    }
    return true;
}
//登陆失败
bool MsgProcess::func_login_failed(tcp_protocol::communication_head &head){
    m_isLogin = false;
    //1.准备数据头
    head.event = tcp_protocol::communication_events::LOGIN_FAILED;
    head.size = 0;
    head.time = MsgUtils::getCurrentTimeInSeconds();
    // std::cout<<m_head.time<<std::endl;
    //2.发送数据
    int remind_data = sizeof(head);
    int ret;
    while(remind_data>0){
        ret = write(m_eventFd,&head+sizeof(head) - remind_data,remind_data);
        if(!MsgUtils::writeError(ret,this->m_epfd,this->m_eventFd)) return false;
        remind_data-=ret;
    }
    return true;
}

//用户登陆
bool MsgProcess::func_login(tcp_protocol::communication_head &head){
    // std::cout<<static_cast<unsigned int>(m_head.event)<<"#"<<m_head.size<<"#"<<m_head.time<<std::endl;
    //1.从data中读取数据
    char buffer[buffer_sizes::LOGIN_BUFFER_SIZE];
    int remind_data = head.size;
    int ret;
    //边沿模式循环读数据
    while(remind_data>0){
        ret = read(this->m_eventFd,buffer+head.size-remind_data,remind_data);
        if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) return false;
        remind_data -= ret;
    }
    //2. data二进制转换成16进制md5
    std::string username_md5 = MsgUtils::toHex(buffer, buffer_sizes::LOGIN_BUFFER_SIZE / 2);
    std::string passwd_md5 = MsgUtils::toHex(buffer + buffer_sizes::LOGIN_BUFFER_SIZE / 2, buffer_sizes::LOGIN_BUFFER_SIZE / 2);
    //TODO：3. 从数据库获取帐号密码数据
    std::string username_true = "123";
    std::string passwd_true = "123";
    //4. 比较两个md5值是否相等
    std::string username_md5_true =  MsgUtils::stringToMD5(username_true);
    std::string passwd_md5_true =  MsgUtils::stringToMD5(passwd_true);
    if(username_md5 != username_md5_true || passwd_md5_true != passwd_md5){
        //5.帐号密码错误，登陆失败
        std::cout<<"登陆失败！"<<std::endl;
        head.size = 0;
        head.event = tcp_protocol::communication_events::LOGIN_FAILED;
        stateTransition(head);
        return false;
    }
    //登陆成功
    std::cout<<"登陆成功！"<<std::endl;
    head.size = 0;
    head.event = tcp_protocol::communication_events::LOGIN_SUCCESS;
    stateTransition(head);
    return true;
}
