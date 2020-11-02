#pragma once

#include "SessionClient.hpp"

namespace simpleApp
{
    class SessionUdp : public SessionClient
    {
        enum class timer_status
        {
            idle,
            connectWait,
            dataWait
        };
    public:
        SessionUdp(int epollfd);
        ~SessionUdp();

        session_result init(std::string address) override;
        session_result proceed(int fd, char* sendMsg, size_t msgSize) override;
        
    private:
        int timerfd = -1;
        bool isTimeout = false;

        timer_status timerStatus = timer_status::idle;

        int timerSet(timer_status status);
    };
}