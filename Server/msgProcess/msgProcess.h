#pragma once
#include "protocol.h"
#include "sys/epoll.h"
#include "msgUtils.h"
#include <map>
#include <list>
#include <mutex>
#include <memory>
#include <atomic>
class MsgProcess{
public:
    MsgProcess(int epfd,int eventFd,std::map<int,MsgProcess*> *fd2MsgProcess):
                m_epfd{epfd},m_eventFd{eventFd},m_isLogin{false},m_fd2MsgProcess{fd2MsgProcess},m_send_filename{nullptr}
                ,m_isReadHead{true}{}
    void run(struct epoll_event &event);

public:
    const std::string FILE_DIR = "/home/lkh/Documents/chatRoom/src/";//文件存储根目录

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
    bool func_client_upload_file(tcp_protocol::communication_head &head);//处理前端发送的文件数据
    bool func_server_send_flie_link();//通知前端渲染下载连接
    bool func_client_request_file(tcp_protocol::communication_head &head);//处理前端发送的下载文件请求
    bool func_server_handler_file();//发送文件给对应fd

    bool process_write_text();//处理发送文本
    bool process_write_file_link();//处理发送文件链接
    bool process_write_file();//处理发送文件

private:
    bool m_isLogin;//是否登陆
    int m_eventFd;//socket对应的fd
    int m_epfd;//epoll fd，用于修改事件
    bool m_isReadHead;
    std::list<std::shared_ptr<std::string>> m_text_buffer;//双向链表存储文本指针
    std::mutex m_text_buffer_mutex;//文本缓冲区锁

    std::list<std::shared_ptr<std::string>> m_file_link_buffer;//双向链表存储代发送文件链接指针
    std::mutex m_file_link_buffer_mutex;//文件指针缓冲区锁

    // std::atomic<std::shared_ptr<std::string>> m_send_file;//一次只能发送一个文件，发送文件时，前端无法请求下载，使用原子变量
    std::shared_ptr<std::string> m_send_filename;
    std::mutex m_filename_mutex;
    std::map<int,MsgProcess*> *m_fd2MsgProcess;//socket fd和MsgProcess索引对 指针
};