#pragma once
#include <iostream>
class MsgUtils{
public:
    static std::string stringToMD5(const std::string& input);
    static std::string toHex(const char* data, size_t length);
    static bool readError(const int &ret,int epfd,int fd);
};