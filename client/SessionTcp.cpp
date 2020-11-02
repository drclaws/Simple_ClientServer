#include "SessionTcp.hpp"

#include <cstring>

#include <sys/socket.h>

#include <unistd.h>

#include <simple_lib/common.h>

namespace simpleApp
{
    SessionTcp::SessionTcp(int epollfd) : SessionClient(epollfd)
    {

    } 

    SessionTcp::~SessionTcp()
    {

    }

    session_result SessionTcp::init(std::string address)
    {
        return this->connectSocket(address, SOCK_STREAM);
    }

    session_result SessionTcp::proceed(int fd, char* sendMsg, size_t msgSize)
    {
        if (fd == STDIN_FILENO)
        {
            if (sendMsg == nullptr || msgSize == 0)
                return session_result(session_status::proceed_msg_empty);
            
            if (this->sendMessage(msg_headers::client_msg, sendMsg, msgSize) == -1)
                return session_result(session_status::proceed_msg_send_fail, errno);
            
            return session_result(session_status::proceed_msg_send);
        }
        else if (fd == this->_socket)
        {
            uint8_t buffer[MESSAGE_MAX_BUFFER];
            auto len = recv(this->_socket, buffer, MESSAGE_MAX_BUFFER, 0);

            if (len == -1)
            {
                return session_result(session_status::proceed_msg_recv_fail, errno);
            }
            else if (len == 0)
            {
                return session_result(session_status::proceed_disconnect);
            }
            else if (static_cast<size_t>(len) <= sizeof(msg_headers))
            {
                return session_result(session_status::proceed_msg_wrong_length);
            }
            
            if (*reinterpret_cast<msg_headers*>(buffer) != msg_headers::server_msg)
            {
                return session_result(session_status::proceed_msg_wrong_length);
            }
            
            return session_result(
                session_status::proceed_msg_recv,
                std::string((char *)(buffer + static_cast<ptrdiff_t>(sizeof(msg_headers))), (size_t)(len - sizeof(msg_headers))));
        }
        else
        {
            return session_result();
        }
    }
} 