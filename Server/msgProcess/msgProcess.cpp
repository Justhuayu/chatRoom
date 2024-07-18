#include "msgProcess.h"
#include "protocol.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
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
    if(!m_isReadHead) return;
    //1. 从read buffer中读取头
    tcp_protocol::communication_head head;
    memset(&head,0,sizeof(head));
    ssize_t ret =read(this->m_eventFd,&head,sizeof(head));
    std::cout<<m_eventFd<<" "<<ret<<" "<<static_cast<unsigned int>(head.event)<<" "<<head.size<<" "<<head.time<<" "<<head.info_size<<std::endl;
    if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) 
    {
        std::cout<<"2"<<std::endl;
        return;
    }
    //2. 根据事件类型，读取数据
    stateTransition(head);
}

//处理写事件
void MsgProcess::processWrite(){
    //发送缓冲区消息
    //发送文本
    if(!this->process_write_text()){
        std::cout<<"fd: "<<m_eventFd<<" send text ERROR!"<<std::endl;
    }
    //发送文件链接
    if(!this->process_write_file_link()){
        std::cout<<"fd: "<<m_eventFd<<" send file link ERROR!"<<std::endl;
    }
    //发送文件
    std::cout<<"⬇⬇########################################⬇⬇"<<std::endl;
    if(!this->process_write_file()){
        std::cout<<"fd: "<<m_eventFd<<" send file ERROR!"<<std::endl;
    }
    std::cout<<"⬆⬆########################################⬆⬆"<<std::endl;

    //3. 写完数据后改成读事件
    //TODO: 不修改为读事件，会阻塞?
    MsgUtils::epoll_mod_event_read(m_epfd,m_eventFd);
}


//处理发送文本
bool MsgProcess::process_write_text(){
    if(m_text_buffer.empty()) return true;
    tcp_protocol::communication_head head;
    memset(&head,0,sizeof(head));
    std::cout<<"send text fd: "<<m_eventFd<<std::endl;
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
        if(!MsgUtils::writeError(ret,m_epfd,m_eventFd)) return false;
        std::cout<<m_eventFd<<" "<<ret<<" "<<static_cast<unsigned int>(head.event)<<" "<<head.size<<" "<<head.time<<" "<<head.info_size<<std::endl;
        //2. 发送写数据
        //循环写事件
        remind_data = send_data->size();
        std::cout<<"发送数据： "<<*send_data<<std::endl;
        const char* data_ptr = send_data->c_str(); // 获取指向数据的指针
        while(remind_data>0){
            ret = write(m_eventFd,data_ptr + send_data->size() - remind_data,remind_data);
            if(!MsgUtils::writeError(ret,this->m_epfd,this->m_eventFd)) return false;
            remind_data -= ret;
        }
    }
    return true;
}
//处理发送文件链接
bool MsgProcess::process_write_file_link(){
    if(m_file_link_buffer.empty()) return true;
    tcp_protocol::communication_head head;
    memset(&head,0,sizeof(head));
    std::cout<<"send file fd: "<<m_eventFd<<std::endl;

    while(!m_file_link_buffer.empty()){
        head.event = tcp_protocol::communication_events::SERVER_SEND_FILE_LINK;
        head.time = MsgUtils::getCurrentTimeInSeconds(); 
        std::string* send_data;
        {
            std::lock_guard<std::mutex> lock(m_file_link_buffer_mutex);
            send_data = m_file_link_buffer.front().get();
            m_file_link_buffer.pop_front();
        }
        std::cout<<"send_data: "<<*send_data<<std::endl;
        head.size = send_data->size();
        //1. 先发送头
        int remind_data = sizeof(head);
        int ret = write(m_eventFd,&head,sizeof(head));
        if(!MsgUtils::writeError(ret,m_epfd,m_eventFd)) return false;
        std::cout<<m_eventFd<<" "<<ret<<" "<<static_cast<unsigned int>(head.event)<<" "<<head.size<<" "<<head.time<<" "<<head.info_size<<std::endl;
        //2. 发送写数据
        //循环写事件
        remind_data = send_data->size();
        const char* data_ptr = send_data->c_str(); // 获取指向数据的指针
        while(remind_data>0){
            ret = write(m_eventFd,data_ptr + send_data->size() - remind_data,remind_data);
            if(!MsgUtils::writeError(ret,this->m_epfd,this->m_eventFd)) return false;
            remind_data -= ret;
        }
    }
    std::cout<<"send file link success!"<<std::endl;

    return true;
}

bool MsgProcess::process_write_file(){
    //发送文件
    std::string *filename;
    {
        std::lock_guard<std::mutex> lock(m_filename_mutex);
        filename = this->m_send_filename.get();
    }
    //没有要发送文件请求
    if(!filename) return true;
    std::string filepath = FILE_DIR + *filename;
    std::cout<<"send file: "<<filepath<<std::endl;
    //1.发送文件头
    tcp_protocol::communication_head head;
    head.event = tcp_protocol::communication_events::SERVER_SEND_FILE;
    head.size = MsgUtils::getFileSize(filepath);
    head.time = MsgUtils::getCurrentTimeInSeconds();
    head.info_size = filename->size();
    int ret = write(m_eventFd,&head,sizeof(head));
    if(!MsgUtils::writeError(ret,m_epfd,m_eventFd)){
        {
            std::lock_guard<std::mutex> lock(m_filename_mutex);
            this->m_send_filename.reset();
        }
        return false;
    } 
    //2. 发送文件名信息
    unsigned long remind_data = head.info_size;
    const char* c_filename = filename->c_str();
    while(remind_data>0){
        ret = write(m_eventFd,c_filename + filename->size() - remind_data,remind_data);
        if(!MsgUtils::writeError(ret,m_epfd,m_eventFd)){
            {
                std::lock_guard<std::mutex> lock(m_filename_mutex);
                this->m_send_filename.reset();
            }
            return false;
        } 
        remind_data -= ret;
        std::cout<<"send filename : "<<ret<<std::endl;
    }
    //3.读文件，发送文件数据
    std::FILE* file = std::fopen(filepath.c_str(),"rb");
    if(!file){
        std::cout<<"Error opening file: " << filepath << std::endl;
        return false;
    }
    char buffer[buffer_sizes::SERVER_SEND_FILE_BUFFER_SIZE];
    remind_data = head.size;
    while(remind_data > 0){
        //1. 从文件读到buffer
        int tmp = std::min(remind_data,sizeof(buffer));
        memset(buffer,0,sizeof(buffer));
        ret = fread(buffer,1,tmp,file);
        if(ret<=0){
            std::cout<<"fread error!"<<std::endl;
            {
                std::lock_guard<std::mutex> lock(m_filename_mutex);
                this->m_send_filename.reset();
            }
            return false;
        }
        //2.socket 发送buffer
        ret = write(m_eventFd,buffer,ret);
        if(!MsgUtils::writeError(ret,m_epfd,m_eventFd)){
            {
                std::lock_guard<std::mutex> lock(m_filename_mutex);
                this->m_send_filename.reset();
            }
            return false;
        } 
        std::cout<<"send file: "<<ret<<std::endl;
        remind_data -= ret;
    }
    std::cout<<"send file success!"<<std::endl;
    //清空发送文件 m_send_filename
    {
        std::lock_guard<std::mutex> lock(m_filename_mutex);
        this->m_send_filename.reset();
    }
    return true;

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
        //客户端发送文本数据请求
        func_client_send_text(head);
        break;
    case tcp_protocol::SERVER_SEND_TEXT:
        //服务器发送文本数据
        func_server_send_text();
        break;
    case tcp_protocol::CLIENT_UPLOAD_FILE:
        //客户端上传文件
        func_client_upload_file(head);
        break;
    case tcp_protocol::SERVER_SEND_FILE_LINK:
        //服务器发送文件链接
        func_server_send_flie_link();
        break;
    case tcp_protocol::CLIENT_REQUEST_FILE:
        //客户端发送文件下载请求
        func_client_request_file(head);
        break;
    case tcp_protocol::SERVER_SEND_FILE:
        //服务端发送文件
        func_server_handler_file();
        break;
    default:
        std::cout<<"status 错误"<<std::endl;
        return false;
        break;
    }
    return true;
}



//处理前端发送的文件数据
bool MsgProcess::func_client_upload_file(tcp_protocol::communication_head &head){
    std::cout<<"recv file info_size :"<<head.info_size<<std::endl;
    //1.读数据，保存到本地
    //if (!m_isLogin) return false;
    //后面需要存储文件，复用文件buffer
    char buffer[buffer_sizes::CLIENT_SEND_FILE_BUFFER_SIZE];
    //1. 读取文件名字
    unsigned long remind_data = head.info_size;
    std::string filename="";
    while(remind_data>0){
        int tmp = std::min(remind_data,sizeof(buffer));
        memset(buffer,0,sizeof(buffer));
        int ret = read(this->m_eventFd,buffer,tmp);
        if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) 
        {
            std::cout<<"1"<<std::endl;
            return false;
        }
        remind_data -= ret;
        std::cout<<"ret: "<<ret<<" buffer: "<<buffer<<std::endl;
        filename.append(buffer,ret);
    }
    //本地保存路径
    std::string filepath = FILE_DIR + filename;
    std::cout<<filepath<<std::endl;
    std::cout<<filename<<std::endl;
    int file_fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd == -1) {
        std::cout<<"Failed to open file"<<std::endl;
        return false;
    }
    //2. 读取文件
    remind_data = head.size;
    m_isReadHead = false;
    while(remind_data>0){
        //从socket读取数据
        int tmp = std::min(remind_data,sizeof(buffer));
        memset(buffer,0,sizeof(buffer));
        int ret = read(this->m_eventFd,buffer,tmp);
        remind_data -= ret;
        std::cout<<"ret: "<<ret<<std::endl;

        if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)) 
        {
            m_isReadHead = true;
            return false;
        }
        // 写入文件
        ssize_t write_ret = write(file_fd,buffer,ret);
        if(write_ret != ret){
            std::cout<<"write file error!"<<std::endl;
            close(file_fd);
                m_isReadHead = true;

            return false;
        }
    }
    m_isReadHead = true;
    close(file_fd);
    //3.文件地址存储到缓冲区
    //TODO：存储到数据库
    std::shared_ptr<std::string> recv_file = std::make_shared<std::string>();
    recv_file->append(filename);
    {
        std::lock_guard<std::mutex> lock(m_file_link_buffer_mutex);
        std::cout<<"recv_file: "<<*recv_file<<std::endl;
        m_file_link_buffer.push_back(recv_file);
    }
    //4. 发送数据
    //广播发送数据
    head.size = 0;
    head.event = tcp_protocol::SERVER_SEND_FILE_LINK;
    stateTransition(head);
    return true;
}

//通知前端渲染下载连接
bool MsgProcess::func_server_send_flie_link()
{
    std::cout<<"通知前端"<<std::endl;
    int fd;
    MsgProcess *msg;
    std::shared_ptr<std::string> send_data;
    {
        std::lock_guard<std::mutex> lock(m_file_link_buffer_mutex);
        send_data = m_file_link_buffer.front();
    }
    for(auto it = m_fd2MsgProcess->begin();it != m_fd2MsgProcess->end();it++){
        fd = it->first;
        msg = it->second;
        //1. 修改所有fd状态为写
        MsgUtils::epoll_mod_event_write(msg->m_epfd,fd);
        //2. 所有fd对应的MsgProcess的文件缓冲区，添加要发送的数据
        if(fd == m_eventFd) continue;//该fd本来就存的有，不用重复添加
        {
            std::lock_guard<std::mutex> lock(msg->m_file_link_buffer_mutex);
            msg->m_file_link_buffer.push_back(send_data);
        }
    }    
    return true;
}


//处理前端发送的下载文件请求
bool MsgProcess::func_client_request_file(tcp_protocol::communication_head &head){
    std::cout<<"recv file download request"<<std::endl;
    //if (!m_isLogin) return false;
    //1. 读取下载文件
    char buffer[buffer_sizes::FILENAME_SIZE_BUFFER_SIZE];
    std::shared_ptr<std::string> filename = std::make_shared<std::string>();
    unsigned long remind_data = head.size;
    while(remind_data>0){
        int tmp = std::min(remind_data,sizeof(buffer));
        memset(buffer,0,sizeof(buffer));
        int ret = read(m_eventFd,buffer,tmp);
        if(!MsgUtils::readError(ret,m_epfd,m_eventFd))
        {
            std::cout<<"3"<<std::endl;
            return false;
        }
        remind_data -= ret;
        filename->append(buffer,ret);
    }
    std::cout<<"request filename: "<<*filename<<std::endl;
    //2. 存储下载文件名
    {
        std::lock_guard<std::mutex> lock(m_filename_mutex);
        if(m_send_filename) std::cout<<"发送文件m_send_filename不为空"<<std::endl;
        m_send_filename = filename;
    }
    //3. 发送下载文件
    head.size = 0;
    head.event = tcp_protocol::SERVER_SEND_FILE;
    stateTransition(head);
    return true;
}

//下载文件
bool MsgProcess::func_server_handler_file()
{
    std::cout<<"下载文件"<<std::endl;
    //修改当前fd为写事件，用于发送文件
    MsgUtils::epoll_mod_event_write(m_epfd,m_eventFd);
    return true;
}

//处理客户端发送到文本消息
bool MsgProcess::func_client_send_text(tcp_protocol::communication_head &head){
    //1. 读数据
    // if(!m_isLogin) return false;
    char buffer[buffer_sizes::CLIENT_SEND_TEXT_BUFFER_SIZE];
    unsigned long remind_data = head.size;
    std::shared_ptr<std::string> recv_text = std::make_shared<std::string>();
    recv_text->reserve(head.size);
    //边沿模式循环读数据
    while(remind_data>0){
        int tmp = std::min(remind_data,sizeof(buffer));
        int ret = read(this->m_eventFd,buffer,tmp);
        if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd)){
            
            std::cout<<"4"<<std::endl;
            return false;
            
        }
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
        if(!MsgUtils::readError(ret,this->m_epfd,this->m_eventFd))
        {
            std::cout<<"5"<<std::endl;
            return false;
        }
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
