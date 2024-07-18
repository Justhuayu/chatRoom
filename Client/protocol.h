#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <cstdint>
//存储前后端通信的协议
namespace tcp_protocol {
enum communication_events{
    LOGIN = 0x01,
    LOGIN_SUCCESS,
    LOGIN_FAILED,
    LOGIN_EXIT,
    CLIENT_SEND_TEXT,
    CLIENT_UPLOAD_FILE,
    CLIENT_REQUEST_FILE,
    SERVER_SEND_TEXT,
    SERVER_SEND_FILE_LINK,
    SERVER_SEND_FILE
};
struct communication_head{
    uint8_t event;//事件描述
    uint64_t size;//包的事件长度
    uint64_t time;//发送数据时间
    uint64_t info_size;//head后面，data前面的数据长度
    //构造函数
    communication_head():info_size{0}{}
}__attribute__((packed));
}

namespace buffer_sizes{
constexpr uint8_t LOGIN_BUFFER_SIZE = 32;
constexpr uint16_t CLIENT_SEND_TEXT_BUFFER_SIZE = 512;
constexpr uint16_t SERVER_SEND_TEXT_BUFFER_SIZE = 512;
constexpr uint16_t FILENAME_SIZE_BUFFER_SIZE = 64;
constexpr uint16_t CLIENT_SEND_FILE_BUFFER_SIZE = 4096;
constexpr uint16_t SERVER_SEND_FILE_BUFFER_SIZE = 4096;

}

#endif // PROTOCOL_H
