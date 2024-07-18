#include "msgUtils.h"
#include "msgProcess.h"
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <chrono>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
// 将二进制数据转换回十六进制字符串
std::string MsgUtils::toHex(const char* data, size_t length) {
    const char hex_digits[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) {
        const unsigned char c = data[i];
        hex.push_back(hex_digits[c >> 4]);
        hex.push_back(hex_digits[c & 0xf]);
    }
    return hex;
}
// 计算MD5哈希值
std::string MsgUtils::stringToMD5(const std::string& input) {
     unsigned char digest[MD5_DIGEST_LENGTH];
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    
    if (context == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(context, EVP_md5(), nullptr) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("Failed to initialize digest context");
    }

    if (EVP_DigestUpdate(context, input.c_str(), input.size()) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("Failed to update digest");
    }

    if (EVP_DigestFinal_ex(context, digest, nullptr) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(context);

    char mdString[2 * MD5_DIGEST_LENGTH + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        sprintf(&mdString[i * 2], "%02x", digest[i]);
    }
    mdString[2 * MD5_DIGEST_LENGTH] = '\0';

    return std::string(mdString);
}
//读取数据是否错误
bool MsgUtils::readError(const int &ret,int epfd,int fd){
    if(ret < 0){
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 数据暂时不可用，等待一段时间再重试
            usleep(1000); // 1毫秒
        }
        std::cout<<"[ERROR]read 数据失败！"<<std::endl;
        return false;
    }else if(ret == 0){
        //对端关闭，修改事件
        //TODO:通过修改事件，在遍历epoll时关闭，当并发很高时，是否会影响性能？
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = EPOLLRDHUP;
        epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
        return false;
    }
    return true;
}

//写数据是否错误
bool MsgUtils::writeError(const int &ret,int epfd,int fd){
    if(ret < 0){
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 数据暂时不可用，等待一段时间再重试
            usleep(1000); // 1毫秒
        }
        std::cout<<"[ERROR]read 数据失败！"<<std::endl;
        return false;
    }else if(ret == 0){
        //对端关闭，修改事件
        //TODO:通过修改事件，在遍历epoll时关闭，当并发很高时，是否会影响性能？
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = EPOLLRDHUP;
        epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
        return false;
    }
    return true;
}

// 获取当前时间的秒数并转换为64位整数
uint64_t MsgUtils::getCurrentTimeInSeconds() {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 获取从纪元时间（1970-01-01 00:00:00 UTC）开始的秒数
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

    // 转换为64位整数
    return static_cast<uint64_t>(duration.count());
}

//设置为非阻塞
void MsgUtils::set_nonblocking(int &fd) {
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

//设置epoll 读
void MsgUtils::epoll_mod_event_read(int &epfd,int &fd){
    struct epoll_event ev;
    MsgUtils::set_nonblocking(fd);
    ev.data.fd = fd;
    ev.events = EPOLLIN|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
}
//设置epoll 写
void MsgUtils::epoll_mod_event_write(int &epfd,int &fd){
    struct epoll_event ev;
    MsgUtils::set_nonblocking(fd);
    ev.data.fd = fd;
    ev.events = EPOLLOUT|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
}

