cmake_minimum_required(VERSION 3.10)

# 项目信息
project(chatRoomServer)

# 包含子目录CMakeLists
add_subdirectory(epoll)
add_subdirectory(msgProcess)
add_subdirectory(protocol)
add_subdirectory(threadPool)
add_subdirectory(mysqlPool)


# 添加可执行文件
add_executable(chatRoomServer main.cpp)

# 链接库
target_link_libraries(chatRoomServer epollServerLib)
