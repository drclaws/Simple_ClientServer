#pragma once

namespace simpleApp
{
    enum class session_status
    {
        // INIT

        init_success,
        init_not_address,
        init_socket_fail,
        init_connection_fail,
        
        init_udp_send_up_fail,
        init_udp_recv_up_fail,
        
        // PROCEED

        proceed_disconnect,
        proceed_server_timeout,
        proceed_server_error,

        proceed_msg,

        proceed_msg_send_fail,
        proceed_msg_recv_fail,
        
        recv_wrong_length,
        recv_wrong_header,
        
        // CONNUP

        connup_up,
        connup_timeout,
        connup_not_req,

        server_error,
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

        session_result(std::string recvMsg) : status(session_status::proceed_msg), err(0), recvMsg(recvMsg)
        {

        }

        session_result() : status(session_status::unknown_status), err(0), recvMsg(std::string(""))
        {

        }
    };
}
