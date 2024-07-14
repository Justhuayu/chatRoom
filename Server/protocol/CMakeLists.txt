# 设置库
add_library(protocolLib INTERFACE)  # 假设 `protocol.h` 是一个头文件库，没有cpp实现文件

# 包含头文件目录
target_include_directories(protocolLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})