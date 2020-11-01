#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h> 
#include <termios.h> 
#include <time.h> 
#include <sys/ioctl.h>

#include <cstring>

#include "Server.hpp"

simpleApp::Server server = simpleApp::Server();

void onExit(int s)
{
    int err = server.raiseStop();
    
    if(err != 0)
        std::cout << "Stop event trigger failed with code " << err << std::endl;
}

int main(int argc, char** argv)
{
    // Hide console input
    termios orig_term_attr; 
    termios new_term_attr;
    tcgetattr(fileno(stdin), &orig_term_attr); 
    memcpy(&new_term_attr, &orig_term_attr, sizeof(termios)); 
    new_term_attr.c_lflag &= ~(ECHO | ICANON); 
    new_term_attr.c_cc[VTIME] = 0; 
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

    std::cout << "Welcome to the Simple Server! For exit press Ctrl+C" << std::endl;
            
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

    return server.serverLoop();
}
