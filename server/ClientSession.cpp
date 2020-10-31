#include "ClientSession.hpp"

#if defined(__linux__)
#include <sys/socket.h>
#include <sys/epoll.h>

#else
#error System is not supported

#endif

#include <unistd.h>

namespace simpleApp
{
    ClientSession::ClientSession(int& epollfd) : epollfd(epollfd)
    {
        
    }

    ClientSession::~ClientSession()
    {
        if (this->_socket != -1)
        {
            epoll_ctl(this->epollfd, EPOLL_CTL_DEL, this->_socket, 0);
            shutdown(this->_socket, SHUT_RDWR);
            close(this->_socket);
        }
        if (this->address != nullptr)
            delete this->address;
    }

    sockaddr_in* ClientSession::getAddress()
    {
        return this->address;
    }
}