#include <iostream>
#include "epollServer.h"
// #include "threadPool.h"
int main(){
    int port = 11294;
    int event_size = 1024;
    EpollServer server(port,event_size);
    server.run();
    // threadPool tp;
}