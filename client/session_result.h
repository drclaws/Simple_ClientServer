#pragma once

namespace simpleApp
{
    enum class session_status
    {
        init_success,
        init_not_address,
        init_socket_fail,
        init_connection_fail,
        init_epoll_add_fail,
        
        init_udp_conn_wait,
        init_udp_send_up_fail,
        init_udp_timer_fail,
        
        proceed_msg_recv,
        proceed_msg_recv_fail,
        proceed_msg_wrong_length,
        proceed_disconnect,
        proceed_msg_send,
        proceed_msg_send_fail,
        proceed_msg_empty,
        proceed_msg_wrong_header,

        proceed_udp_connected,
        proceed_udp_timeout,

        udp_timer_set_fail,
        unknown_status
    };

    struct session_result
    {
        session_status status;
        int err;
        std::string recvMsg;

        session_result(session_status status, int err = 0) : status(status), err(err), recvMsg(std::string(""))
        {

        }

        session_result(session_status status, std::string recvMsg) : status(status), err(0), recvMsg(recvMsg)
        {

        }

        session_result() : status(session_status::unknown_status), err(0), recvMsg(std::string(""))
        {

        }
    };
}
