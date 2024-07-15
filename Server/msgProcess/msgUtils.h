#pragma once
#include <iostream>
class MsgUtils{
public:
    static uint64_t getCurrentTimeInSeconds();//获取当前时间秒数
    static std::string stringToMD5(const std::string& input);//string计算MD5哈希值
    static std::string toHex(const char* data, size_t length);//二进制数据转换回十六进制字符串
    static bool readError(const int &ret,int epfd,int fd);//读取数据是否错误
};