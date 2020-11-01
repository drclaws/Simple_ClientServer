#pragma once

#include <sys/timerfd.h>

#include "Session.hpp"
#include "session_result.h"

namespace simpleApp
{
    class SessionUdp : public Session
    {
    public:
        SessionUdp(int epollfd);
        ~SessionUdp();

        session_result init(socket_t masterSocket, uint16_t port) override;
        
        session_result proceed() override;
        
    private:
        int timerfd = -1;
        bool timerReset();
        bool isTimeout = false;
    };
}
