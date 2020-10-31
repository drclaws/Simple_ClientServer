#pragma once

#include <sys/timerfd.h>

#include "ClientSession.hpp"
#include "session_result.h"

namespace simpleApp
{
    class UdpSession : public ClientSession
    {
    public:
        UdpSession(int& epollfd);
        ~UdpSession();

        session_result init(socket_t masterSocket, uint16_t port) override;
        
        session_result proceed(struct epoll_event& epoll_event) override;
        
    private:
        int timerfd = -1;
        bool timerReset();
    };
}
