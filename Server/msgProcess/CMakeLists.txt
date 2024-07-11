# 设置库
add_library(msgProcessLib msgProcess.cpp)

# 包含头文件目录
target_include_directories(msgProcessLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})