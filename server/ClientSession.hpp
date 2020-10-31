#pragma once

#include <netinet/in.h>
#include <simple_lib/common.h>

#include "session_result.h"

namespace simpleApp
{
    class ClientSession 
    {
    public:
        ~ClientSession();
        
        virtual session_result init(socket_t masterSocket, uint16_t port) = 0;
        virtual session_result proceed(struct epoll_event& epoll_event) = 0;
        
        sockaddr_in* getAddress();
        
    protected:
        socket_t _socket = -1;
        int& epollfd;      
        sockaddr_in * address = nullptr;

        ClientSession(int& epollfd);
    };
}