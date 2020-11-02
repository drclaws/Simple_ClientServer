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

        virtual session_result init(in_addr_t address) = 0;
        session_result proceed(const char* sendMsg = nullptr, size_t msgSize = 0);
        virtual session_result sendConnup();
        socket_t getSocket();
        
    protected:
        SessionClient();
        
        session_result connectSocket(in_addr_t address, int sockType);
    };
}