#pragma once

#include <simple_lib/common.h>

#include "SessionServer.hpp"
#include "session_result.h"

namespace simpleApp
{
    class SessionTcp : public SessionServer
    {
    public:
        SessionTcp(int epollfd);
        ~SessionTcp();

        session_result init(socket_t masterSocket) override;
        
        session_result proceed() override;
    };
}
