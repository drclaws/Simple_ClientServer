#pragma once

#include "SessionClient.hpp"

namespace simpleApp
{
    class SessionUdp : public SessionClient
    {
    public:
        SessionUdp();
        ~SessionUdp();

        session_result init(in_addr_t address) override;
        session_result sendConnup() override;
    };
}