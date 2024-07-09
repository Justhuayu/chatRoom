cmake_minimum_required(VERSION 3.9) #最低版本

project(chatRoomServer VERSION 1.0) #项目名称及自定义版本号

include_directories(${CMAKE_SOURCE_DIR}/epoll)#添加子文件
add_subdirectory(epoll)

add_executable(chatRoomServer main.cpp) #添加生成可执行文件
