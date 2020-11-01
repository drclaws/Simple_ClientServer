#pragma once

namespace simpleApp 
{
    class Server
    {
    public:
        Server();  
        ~Server();

        int serverLoop();

        int initStop();

        int raiseStop();

    private:
        int stopEventFd = -1;
    };
} // namespace simpleApp
