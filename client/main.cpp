#include <iostream>

#include <signal.h>

#include "MainConsole.hpp"

simpleApp::MainConsole console = simpleApp::MainConsole();

void onBreak(int s)
{
    int err = console.raiseBreak();
    
    if(err != 0)
        std::cout << "Break event trigger failed with code " << err << std::endl;
}

int main(int argc, char** argv)
{
    std::cout << "Welcome to the Simple Client! For exit press Ctrl+C" << std::endl;

    int err = console.initBreak();
    if (err != 0)
    {
        std::cout << "Break event initialization failed with code " << err << std::endl;
        return -1;
    }

    struct sigaction sigIntHandler;
    
    sigIntHandler.sa_handler = onBreak;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    if(sigaction(SIGINT, &sigIntHandler, NULL) == -1)
    {
        std::cout << "Break event configuration failed with code " << errno << std::endl;
        return -1;
    }

    return console.consoleLoop();
}
