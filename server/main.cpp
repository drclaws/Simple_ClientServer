#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "Server.hpp"

simpleApp::Server server = simpleApp::Server();

void onExit(int s)
{
    int err = server.stop();
    
    if(err != 0)
        std::cout << "Stop event trigger failed with code " << err << std::endl;
}

int main(int argc, char** argv)
{
    std::cout << "Welcome to the SimpleServer!" << std::endl <<
        "For exit press Ctrl+C" << std::endl;
            
    int err = server.initStop();
    if(err != 0)
    {
        std::cout << "Stop event initialization failed with code " << err << std::endl;
        return -1;
    }

    struct sigaction sigIntHandler;
    
    sigIntHandler.sa_handler = onExit;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    if(sigaction(SIGINT, &sigIntHandler, NULL) == -1)
    {
        std::cout << "Stop event configuration failed with code " << errno << std::endl;
        return -1;
    }

    return server.serverLoop(static_cast<uint16_t>(35831));
}
