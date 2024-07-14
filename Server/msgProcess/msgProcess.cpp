#include "msgProcess.h"
#include "protocol.h"
#include <iostream>
#include <iostream>
#include <unistd.h>
#include <cstring>
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
    tcp_protocol::communication_head head;
    int ret = read(this->m_eventFd,&head,sizeof(head));
    if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) return;
    //2. 根据事件类型，读取数据
    stateTransition(static_cast<tcp_protocol::communication_events>(head.event));
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
    default:
        std::cout<<"status 错误"<<std::endl;
        return false;
        break;
    }
    return true;
}

//登陆成功
bool MsgProcess::func_login_success(){

}
//登陆失败
bool MsgProcess::func_login_failed(){

}

//用户登陆
bool MsgProcess::func_login(){
    //1.从data中读取数据
    char buffer[buffer_sizes::LOGIN_BUFFER_SIZE];
    int ret = read(this->m_eventFd,buffer,sizeof(buffer));
    if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) return false;
    //2. data二进制转换成16进制md5
    std::string username_md5 = MsgUtils::toHex(buffer, buffer_sizes::LOGIN_BUFFER_SIZE / 2);
    std::string passwd_md5 = MsgUtils::toHex(buffer + buffer_sizes::LOGIN_BUFFER_SIZE / 2, buffer_sizes::LOGIN_BUFFER_SIZE / 2);
    //3. TODO：从数据库获取帐号密码数据
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
