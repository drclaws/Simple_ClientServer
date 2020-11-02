#include "SessionUdp.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>

#include <unistd.h>

#include <simple_lib/common.h>

namespace simpleApp
{
    SessionUdp::SessionUdp() : SessionClient()
    {

    }

    SessionUdp::~SessionUdp()
    {
        if (this->_socket != -1)
        {
            this->sendMessage(msg_headers::client_conndown);
        }
    }

    session_result SessionUdp::init(in_addr_t address)
    {
        auto baseInitResult = this->connectSocket(address, SOCK_DGRAM);

        if (baseInitResult.status != session_status::init_success)
            return baseInitResult;

        if (this->sendMessage(msg_headers::req_connstart) == -1)
        {
            auto result = session_result(session_status::init_udp_send_up_fail, errno);
            this->sessionClose();
            return result;
        }
        
        uint8_t buffer[MESSAGE_MAX_BUFFER];
        auto len = recv(this->_socket, buffer, MESSAGE_MAX_BUFFER, MSG_NOSIGNAL);

        if (len == -1)
        {
            auto result = session_result(session_status::init_udp_recv_up_fail, errno);
            this->sessionClose();
            return result;
        }
        else if (len != sizeof(msg_headers))
        {
            this->sessionClose();
            return session_result(session_status::recv_wrong_length);
        }
        
        msg_headers header = *reinterpret_cast<msg_headers*>(buffer);
        
        if (header == msg_headers::err_connstart)
        {
            this->sessionClose();
            return session_result(session_status::server_error);
        }
        
        if (header != msg_headers::accept_connstart)
        {
            this->sessionClose();
            return session_result(session_status::recv_wrong_header);
        }
        
        return session_result(session_status::init_success);
    }

    session_result SessionUdp::sendConnup()
    {
        if(this->sendMessage(msg_headers::client_connup) == -1)
        {
            return session_result(session_status::proceed_msg_send_fail, errno);
        }

        uint8_t buffer[sizeof(msg_headers) + 1];
        auto len = recv(this->_socket, buffer, sizeof(msg_headers) + 1, MSG_NOSIGNAL);

        if (len == -1)
        {
            return session_result(session_status::connup_timeout, errno);
        }
        else if (len != sizeof(msg_headers))
        {
            return session_result(session_status::recv_wrong_length);
        }
        
        msg_headers header = *reinterpret_cast<msg_headers*>(buffer);
        
        return session_result(
            header == msg_headers::server_connup
            ? session_status::connup_up
            : session_status::recv_wrong_header);
    }
}