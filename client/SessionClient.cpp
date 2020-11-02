#include "SessionClient.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <strings.h>

#include <unistd.h>

#include <simple_lib/common.h>

namespace simpleApp
{
    SessionClient::SessionClient() : Session()
    {

    }

    SessionClient::~SessionClient()
    {
        
    }

    session_result SessionClient::connectSocket(in_addr_t address, int sockType)
    {
        this->_socket = socket(AF_INET, sockType, 0);
        if (this->_socket == -1)
        {
            return session_result(session_status::init_socket_fail, errno);
        }

        timeval tv;
        tv.tv_sec = WAIT_TIMEOUT_SEC;
        tv.tv_usec = 0;
        setsockopt(this->_socket, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(tv));
        setsockopt(this->_socket, SOL_SOCKET, SO_SNDTIMEO, (char*) &tv, sizeof(tv));

        sockaddr_in addrStruct;
        bzero(&addrStruct, sizeof(addrStruct));
        addrStruct.sin_addr.s_addr = address;
        addrStruct.sin_port = htons(PUBLIC_PORT);
        addrStruct.sin_family = AF_INET;

        if (connect(this->_socket, (sockaddr *)&addrStruct, sizeof(addrStruct)) == -1)
        {
            auto result = session_result(session_status::init_connection_fail, errno);
            close(this->_socket);
            this->_socket = -1;
            return result;
        }

        return session_result(session_status::init_success);
    }

    session_result SessionClient::proceed(char* sendMsg, size_t msgSize)
    {
        if(sendMsg == nullptr || msgSize == 0)
        {
            uint8_t buffer[sizeof(msg_headers) + 1];
            auto len = recv(this->_socket, buffer, sizeof(msg_headers) + 1, 0);

            if (len == -1)
            {
                return session_result(session_status::proceed_msg_recv_fail, errno);
            }
            else if (len == 0)
            {
                return session_result(session_status::proceed_disconnect);
            }
            else if (static_cast<size_t>(len) != sizeof(msg_headers))
            {
                return session_result(session_status::recv_wrong_length);
            }

            msg_headers header = *reinterpret_cast<msg_headers*>(buffer);

            switch (header)
            {
            case msg_headers::server_conndown:
                return session_result(session_status::proceed_disconnect);
            case msg_headers::server_session_err:
                return session_result(session_status::proceed_server_error);
            case msg_headers::server_timeout:
                return session_result(session_status::proceed_server_timeout);
            default:
                return session_status(session_status::unknown_status);
            }
        }
        else if (sendMsg != nullptr && msgSize != 0)
        {
            // TODO
            return session_status();
        }
        else
        {
            return session_result(session_status::unknown_status);
        }
    }

    socket_t SessionClient::getSocket()
    {
        return this->_socket;
    }

    session_result SessionClient::sendConnup()
    {
        return session_result(session_status::connup_not_req);
    }
}