cmake_minimum_required(VERSION 3.10)

# 添加你的源文件
add_library(epollServerLib epollServer.cpp)

# 设置包含目录
target_include_directories(epollServerLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接库
target_link_libraries(epollServerLib  msgProcessLib threadPoolLib)