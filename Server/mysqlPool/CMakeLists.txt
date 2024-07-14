# 设置库
add_library(mysqlPoolLib mysqlPool.cpp)  

# 包含头文件目录
target_include_directories(mysqlPoolLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
