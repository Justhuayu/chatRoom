#pragma once
#include <iostream>
class MsgUtils{
public:
    static uint64_t getCurrentTimeInSeconds();//获取当前时间秒数
    static std::string stringToMD5(const std::string& input);//string计算MD5哈希值
    static std::string toHex(const char* data, size_t length);//二进制数据转换回十六进制字符串
    static bool readError(const int &ret,int epfd,int fd);//读取数据是否错误
    static bool writeError(const int &ret,int epfd,int fd);//写数据是否错误
    static void set_nonblocking(int &fd);//设置fd为非阻塞
    static void epoll_mod_event_read(int &epfd,int &fd);//设置事件为读
    static void epoll_mod_event_write(int &epfd,int &fd);//设置事件为写
    static uint64_t getFileSize(const std::string& filepath);//获取文件大小

};