cmake_minimum_required(VERSION 3.10) #最低版本

project(chatRoomServer VERSION 1.0) #项目名称及自定义版本号

# 添加子目录
add_subdirectory(epoll)
add_subdirectory(threadPool)

# 设置包含目录
include_directories(${CMAKE_SOURCE_DIR}/epoll)
include_directories(${CMAKE_SOURCE_DIR}/threadPool)

# 添加源文件
set(SOURCE_FILES
    main.cpp
)

# 添加可执行文件
add_executable(chatRoomServer ${SOURCE_FILES})

# 链接 epollServer 库
target_link_libraries(chatRoomServer 
                        epollServer 
                        threadPool)
