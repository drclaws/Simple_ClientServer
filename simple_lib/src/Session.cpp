#include "../include/simple_lib/Session.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>

#include <unistd.h>

namespace simpleApp
{
    Session::Session(int epollfd) : epollfd(epollfd)
    {
        
    }

    Session::~Session()
    {
        this->sessionClose();
    }

    void Session::sessionClose()
    {
        if (this->_socket != -1)
        {
            epoll_ctl(this->epollfd, EPOLL_CTL_DEL, this->_socket, 0);
            shutdown(this->_socket, SHUT_RDWR);
            close(this->_socket);
            this->_socket = -1;
        }
    }
}