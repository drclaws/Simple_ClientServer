#pragma once

#include <simple_lib/common.h>

#include "Session.hpp"
#include "session_result.h"

namespace simpleApp
{
    class SessionTcp : public Session
    {
    public:
        SessionTcp(int epollfd);
        ~SessionTcp();

        session_result init(socket_t masterSocket, uint16_t port) override;
        
        session_result proceed() override;
    };
}
