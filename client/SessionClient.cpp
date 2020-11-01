#include "SessionClient.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>

#include <unistd.h>

namespace simpleApp
{
    SessionClient::SessionClient(int epollfd) : Session(epollfd)
    {

    }

    SessionClient::~SessionClient()
    {
        
    }
}