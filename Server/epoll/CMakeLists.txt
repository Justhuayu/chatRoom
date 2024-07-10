#epoll/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(epollServer)

include_directories(${CMAKE_SOURCE_DIR}/threadPool)

# 添加你的源文件
add_library(epollServer epollServer.cpp)
target_link_libraries(epollServer threadPool)