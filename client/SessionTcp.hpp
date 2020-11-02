#pragma once

#include "SessionClient.hpp"

namespace simpleApp
{
    class SessionTcp : public SessionClient
    {
    public:
        SessionTcp(int epollfd);
        ~SessionTcp();

        session_result init(std::string address) override;
        session_result proceed(int fd, char* sendMsg, size_t msgSize) override;
    };
}