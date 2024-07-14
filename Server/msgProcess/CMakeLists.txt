# 查找 OpenSSL 库
find_package(OpenSSL REQUIRED)

# 设置库
add_library(msgProcessLib msgProcess.cpp msgUtils.cpp)

# 包含头文件目录
target_include_directories(msgProcessLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(msgProcessLib protocolLib OpenSSL::SSL OpenSSL::Crypto)