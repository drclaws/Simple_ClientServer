#pragma once

#include <string>

#if defined(__linux__)
#include <netinet/in.h>

#else
#error System is not supported

#endif

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
        
        std::string getName();
        
    protected:
        socket_t _socket = -1;
        int& epollfd;
        std::string _name;

        ClientSession(int& epollfd, std::string name = "");

        void sessionClose();
    };

    std::string addressToString(sockaddr_in& address);
}