#pragma once

#include "common.h"

namespace simpleApp
{
    class Session
    {
    public:
        virtual ~Session();

    protected:
        socket_t _socket = -1;
        int epollfd;

        Session(int epollfd);

        void sessionClose();
    };
}