# 设置库
add_library(threadPoolLib INTERFACE)  # 假设 `threadPool.h` 也是一个头文件库，没有cpp实现文件

# 包含头文件目录
target_include_directories(threadPoolLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(threadPoolLib INTERFACE mysqlPoolLib)