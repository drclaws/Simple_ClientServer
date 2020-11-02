#include "../include/simple_lib/Session.hpp"

#include <cstring>

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

    int Session::sendMessage(msg_headers header, char* msg, size_t msgSize)
    {
        uint8_t buffer[MESSAGE_MAX_BUFFER];
        
        std::memcpy(buffer, &header, sizeof(header));
        
        if (msg != nullptr && msgSize > 0)
        {
            std::memcpy(buffer + static_cast<ptrdiff_t>(sizeof(header)), msg, msgSize);
            return send(this->_socket, buffer, sizeof(header) + msgSize, 0);
        }
        else
        {
            return send(this->_socket, buffer, sizeof(header), 0);
        }
    }
    
}