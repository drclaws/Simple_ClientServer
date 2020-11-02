#pragma once

#include <stdint.h>

namespace simpleApp 
{
    enum class session_status
    {
        // Session init group
        init_success,
        init_listener_fail,         // Master socket reading failed
        init_socket_setup_fail,
        init_udp_wrong_length,
        init_udp_wrong_header,
        init_udp_timer_fail,

        // Data proceeding group
        proceed_msg_send,
        proceed_disconnect,
        proceed_recv_fail,
        proceed_send_fail,
        proceed_unknown_fd,
        proceed_wrong_header,

        proceed_tcp_wrong_size,

        proceed_udp_timer_fail,
        proceed_recv_timeout,
        proceed_udp_false_timeout,
        proceed_udp_client_timeout,
        proceed_udp_socket_was_closed,
        proceed_udp_connup,

        unknown_status
    };

    struct session_result
    {
        session_status status;
        int err;

        session_result(session_status status, int err = 0) : status(status), err(err)
        {

        }

        session_result() : status(session_status::unknown_status), err(0)
        {
            
        }

    };
}