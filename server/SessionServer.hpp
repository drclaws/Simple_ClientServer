#pragma once

#include <string>

#include <netinet/in.h>
#include <sys/epoll.h>


#include <simple_lib/common.h>
#include <simple_lib/Session.hpp>

#include "session_result.h"

namespace simpleApp
{
    class SessionServer : public Session
    {
    public:
        virtual ~SessionServer();
        
        virtual session_result init(socket_t masterSocket) = 0;
        virtual session_result proceed() = 0;
        
        std::string getName();
        
    protected:
        std::string _name;

        SessionServer(int epollfd, std::string name = "");
    };

    std::string addressToString(sockaddr_in& address);
}