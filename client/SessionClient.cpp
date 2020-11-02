#include "SessionClient.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <strings.h>

#include <unistd.h>

#include <simple_lib/common.h>

namespace simpleApp
{
    SessionClient::SessionClient(int epollfd) : Session(epollfd)
    {

    }

    SessionClient::~SessionClient()
    {
        
    }

    in_addr_t SessionClient::toInetAddress(std::string addressLine)
    {
        return inet_addr(addressLine.c_str());
    }

    session_result SessionClient::connectSocket(std::string address, int sockType)
    {
        auto convertedAddr = SessionClient::toInetAddress(address);
        if (convertedAddr == INADDR_NONE)
        {
            return session_result(session_status::init_not_address, errno);
        }

        this->_socket = socket(AF_INET, sockType, 0);
        if (this->_socket == -1)
        {
            return session_result(session_status::init_socket_fail, errno);
        }

        if (set_nonblock(this->_socket) == -1)
        {
            auto result = session_result(session_status::init_socket_fail, errno);
            close(this->_socket);
            this->_socket = -1;
            return result;
        }

        sockaddr_in addrStruct;
        bzero(&addrStruct, sizeof(addrStruct));
        addrStruct.sin_addr.s_addr = convertedAddr;
        addrStruct.sin_port = htons(PUBLIC_PORT);
        addrStruct.sin_family = AF_INET;

        if (connect(this->_socket, (sockaddr *)&addrStruct, sizeof(addrStruct)) == -1)
        {
            auto result = session_result(session_status::init_connection_fail, errno);
            close(this->_socket);
            this->_socket = -1;
            return result;
        }

        epoll_event socketEvent;
        socketEvent.data.fd = this->_socket;
        socketEvent.events = EPOLLET | EPOLLIN;

        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, this->_socket, &socketEvent) == -1)
        {
            auto result = session_result(session_status::init_epoll_add_fail, errno);
            shutdown(this->_socket, SHUT_RDWR);
            close(this->_socket);
            this->_socket = -1;
            return result;
        }

        return session_result(session_status::init_success);
    }
}