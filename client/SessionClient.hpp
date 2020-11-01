#pragma once

#include <simple_lib/common.h>
#include <simple_lib/Session.hpp>

namespace simpleApp
{
    class SessionClient : public Session
    {
    public:
        virtual ~SessionClient();

        virtual int init() = 0;
        virtual int proceed(int fd) = 0;
        
    protected:
        SessionClient(int epollfd);
    };
}