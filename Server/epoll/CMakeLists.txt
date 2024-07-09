#epoll/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(epollServer)

# 添加你的源文件
add_library(epollServer epollServer.cpp)