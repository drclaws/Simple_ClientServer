#include "SessionServer.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>

#include <unistd.h>

namespace simpleApp
{
    SessionServer::SessionServer(int epollfd, std::string name) : Session(epollfd), _name(name)
    {
        
    }

    SessionServer::~SessionServer()
    {
        
    }

    std::string SessionServer::getName()
    {
        return this->_name;
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