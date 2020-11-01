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
    ClientSession::ClientSession(int epollfd, std::string name) : epollfd(epollfd), _name(name)
    {
        
    }

    ClientSession::~ClientSession()
    {
        this->sessionClose();
    }

    std::string ClientSession::getName()
    {
        return this->_name;
    }

    void ClientSession::sessionClose()
    {
        if (this->_socket != -1)
        {
            epoll_ctl(this->epollfd, EPOLL_CTL_DEL, this->_socket, 0);
            shutdown(this->_socket, SHUT_RDWR);
            close(this->_socket);
            this->_socket = -1;
        }
    }

    std::string addressToString(sockaddr_in& address)
    {
        uint32_t addressConverted = ntohl(address.sin_addr.s_addr);
        auto portConverted = ntohs(address.sin_port);

        uint8_t* addressByBytes = reinterpret_cast<uint8_t*>(&addressConverted);

        return std::to_string(static_cast<int>(addressByBytes[0])) + std::string(".") +
            std::to_string(static_cast<int>(addressByBytes[1])) + std::string(".") +
            std::to_string(static_cast<int>(addressByBytes[2])) + std::string(".") +
            std::to_string(static_cast<int>(addressByBytes[3])) + std::string(":") +
            std::to_string(portConverted);
    }
}