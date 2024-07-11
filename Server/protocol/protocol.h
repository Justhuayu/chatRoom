#ifndef PROTOCOL_H
#define PROTOCOL_H

//存储前后端通信的协议
namespace tcp_protocol {
    enum communication_events{
        LOGIN = 0x10,
        LOGIN_EXIT,
    };
}

#endif // PROTOCOL_H
