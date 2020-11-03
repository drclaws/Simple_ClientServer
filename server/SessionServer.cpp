#include "SessionServer.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>

#include <unistd.h>

namespace simpleApp
{
    SessionServer::SessionServer(int epollfd, std::string name) : Session(), epollfd(epollfd), _name(name)
    {
        
    }

    SessionServer::~SessionServer()
    {
        
    }

    std::string SessionServer::getName()
    {
        return this->_name;
    }

    void SessionServer::sessionClose()
    {
        if (this->_socket != -1)
        {
            epoll_ctl(this->epollfd, EPOLL_CTL_DEL, this->_socket, 0);
        }
        Session::sessionClose();
    }

    std::string addressToString(sockaddr_in& address)
    {
        auto portConverted = ntohs(address.sin_port);

        uint8_t* addressByBytes = reinterpret_cast<uint8_t*>(&address.sin_addr.s_addr);

        return std::to_string(static_cast<int>(addressByBytes[0])) + std::string(".") +
            std::to_string(static_cast<int>(addressByBytes[1])) + std::string(".") +
            std::to_string(static_cast<int>(addressByBytes[2])) + std::string(".") +
            std::to_string(static_cast<int>(addressByBytes[3])) + std::string(":") +
            std::to_string(portConverted);
    }
}