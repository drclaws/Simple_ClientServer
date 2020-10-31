#pragma once

#include <simple_lib/common.h>

#include "ClientSession.hpp"
#include "session_result.h"

namespace simpleApp
{
    class TcpSession : public ClientSession
    {
    public:
        TcpSession(int& epollfd);
        ~TcpSession();

        session_result init(socket_t masterSocket, uint16_t port) override;
        
        session_result proceed(struct epoll_event& epoll_event) override;
    };
}
