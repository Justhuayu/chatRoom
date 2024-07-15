#include "msgProcess.h"
#include "protocol.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <algorithm>
void MsgProcess::run(struct epoll_event event){
    if(this->m_eventFd != event.data.fd){
        std::cout<<"[ERROR]MsgProcess 中 fd 和待处理事件 fd 不相等！"<<std::endl;
        return;
    }
    if(event.events == EPOLLIN){
        //读事件
        processRead();
    }else if(event.events == EPOLLOUT){
        //写事件
        processWrite();
    }else{
        //TODO：处理其它事件
    }
}
//处理读事件
void MsgProcess::processRead(){
    //1. 从read buffer中读取头
    memset(&m_head,0,sizeof(m_head));
    int ret = read(this->m_eventFd,&m_head,sizeof(m_head));
    std::cout<<m_eventFd<<" "<<ret<<" "<<static_cast<unsigned int>(m_head.event)<<" "<<m_head.size<<" "<<m_head.time<<std::endl;
    if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) return;
    //2. 根据事件类型，读取数据
    stateTransition(static_cast<tcp_protocol::communication_events>(m_head.event));
}

//处理写事件
void MsgProcess::processWrite(){

}
//消息处理入口函数，状态机调用其它状态
bool MsgProcess::stateTransition(const tcp_protocol::communication_events &event){
    switch (event)
    {
    case tcp_protocol::LOGIN:
        //登陆
        func_login();
        break;
    case tcp_protocol::LOGIN_SUCCESS:
        //登陆成功
        func_login_success();
        break;
    case tcp_protocol::LOGIN_FAILED:
        func_login_failed();
        //登陆失败
        break;
    case tcp_protocol::LOGIN_EXIT:
        //退出登陆
        break;
    case tcp_protocol::CLIENT_SEND_TEXT:
        //发送文本数据请求
        func_send_file();
        break;
    default:
        std::cout<<"status 错误"<<std::endl;
        return false;
        break;
    }
    return true;
}
bool MsgProcess::func_send_file(){
    char buffer[buffer_sizes::CLIENT_SEND_TEXT_BUFFER_SIZE];
    uint64_t remind_data = m_head.size;
    //文本数据，直接用string接收
    std::string recv_data;
    recv_data.reserve(m_head.size);
    //边沿模式循环读数据
    while(remind_data>0){
        int tmp = remind_data < sizeof(buffer)?remind_data:sizeof(buffer);
        int ret = read(this->m_eventFd,buffer,tmp);
        if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) return false;
        remind_data -= ret;
        recv_data.append(buffer);
    }
    std::cout<<recv_data<<std::endl;
    return true;
}

//登陆成功
bool MsgProcess::func_login_success(){   
    m_isLogin = true;
    //发送数据
    //1.准备数据头
    m_head.event = tcp_protocol::communication_events::LOGIN_SUCCESS;
    //TODO: 成功发送历史聊天记录里
    m_head.size = 0;
    m_head.time = MsgUtils::getCurrentTimeInSeconds();
    std::cout<<m_head.time<<std::endl;
    //2.发送数据
    int remind_data = sizeof(m_head);
    int ret;
    while(remind_data>0){
        ret = write(m_eventFd,&m_head+sizeof(m_head) - remind_data,remind_data);
        //TODO: 写数据失败 MsgUtils::writeError
        remind_data-=ret;
    }
    return true;
}
//登陆失败
bool MsgProcess::func_login_failed(){
    m_isLogin = false;
    //1.准备数据头
    m_head.event = tcp_protocol::communication_events::LOGIN_FAILED;
    m_head.size = 0;
    m_head.time = MsgUtils::getCurrentTimeInSeconds();
    std::cout<<m_head.time<<std::endl;
    //2.发送数据
    int remind_data = sizeof(m_head);
    int ret;
    while(remind_data>0){
        ret = write(m_eventFd,&m_head+sizeof(m_head) - remind_data,remind_data);
        remind_data-=ret;
    }
    return true;
}

//用户登陆
bool MsgProcess::func_login(){
    std::cout<<static_cast<unsigned int>(m_head.event)<<"#"<<m_head.size<<"#"<<m_head.time<<std::endl;
    //1.从data中读取数据
    char buffer[buffer_sizes::LOGIN_BUFFER_SIZE];
    int remind_data = m_head.size;
    int ret;
    //边沿模式循环读数据
    while(remind_data>0){
        ret = read(this->m_eventFd,buffer+m_head.size-remind_data,remind_data);
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
        stateTransition(tcp_protocol::communication_events::LOGIN_FAILED);
        return false;
    }
    //登陆成功
    std::cout<<"登陆成功！"<<std::endl;
    stateTransition(tcp_protocol::LOGIN_SUCCESS);
    return true;
}
