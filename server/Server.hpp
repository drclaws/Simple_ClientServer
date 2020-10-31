#pragma once

#include <stdint.h>
#include <stdlib.h>

namespace simpleApp 
{
    class Server
    {
    public:
        Server();  
        ~Server();

        int serverLoop(uint16_t port);

        int initStop();

        int stop();

    private:
        int stopObject = -1;
    };
} // namespace simpleApp
