#pragma once

#include <stdint.h>

namespace simpleApp 
{
    class Server
    {
    public:
        Server();  
        ~Server();

        int serverLoop(uint16_t port);

        int initStop();

        int raiseStop();

    private:
        int stopEventFd = -1;
    };
} // namespace simpleApp
