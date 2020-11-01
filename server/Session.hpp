#pragma once

#include <string>

#include <netinet/in.h>
#include <sys/epoll.h>


#include <simple_lib/common.h>

#include "session_result.h"

namespace simpleApp
{
    class Session 
    {
    public:
        virtual ~Session();
        
        virtual session_result init(socket_t masterSocket, uint16_t port) = 0;
        virtual session_result proceed() = 0;
        
        std::string getName();
        
    protected:
        socket_t _socket = -1;
        int epollfd;
        std::string _name;

        Session(int epollfd, std::string name = "");

        void sessionClose();
    };

    std::string addressToString(sockaddr_in& address);
}