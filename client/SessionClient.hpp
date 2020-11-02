#pragma once

#include <string>

#include <simple_lib/common.h>
#include <simple_lib/Session.hpp>

#include "session_result.h"

#include <netinet/in.h>

namespace simpleApp
{
    class SessionClient : public Session
    {
    public:
        virtual ~SessionClient();

        virtual session_result init(std::string address) = 0;
        virtual session_result proceed(int fd, char* sendMsg = nullptr, size_t msgSize = 0) = 0;
        
    protected:
        SessionClient(int epollfd);
        
        static in_addr_t toInetAddress(std::string addressLine);
        
        session_result connectSocket(std::string address, int sockType);
    };
}