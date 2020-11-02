#pragma once

#include <sys/timerfd.h>

#include "SessionServer.hpp"
#include "session_result.h"

namespace simpleApp
{
    class SessionUdp : public SessionServer
    {
    public:
        SessionUdp(int epollfd);
        ~SessionUdp();

        session_result init(socket_t masterSocket) override;
        
        session_result proceed() override;

    private:
        int timerfd = -1;
        bool timerReset();
        bool isTimeout = false;
    };
}
