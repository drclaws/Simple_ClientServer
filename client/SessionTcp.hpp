#pragma once

#include "SessionClient.hpp"

namespace simpleApp
{
    class SessionTcp : public SessionClient
    {
    public:
        SessionTcp();
        ~SessionTcp();

        session_result init(in_addr_t address) override;
    };
}