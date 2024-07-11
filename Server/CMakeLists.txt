cmake_minimum_required(VERSION 3.10)

# 项目信息
project(chatRoom)

# 包含子目录CMakeLists
add_subdirectory(epoll)
add_subdirectory(msgProcess)
add_subdirectory(protocol)
add_subdirectory(threadPool)

# 添加可执行文件
add_executable(chatRoom main.cpp)

# 链接库
target_link_libraries(chatRoom epollServerLib)
