#pragma once

#include <stdint.h>

namespace simpleApp 
{
    enum class session_status
    {
        init_success,
        init_socket_setup_fail,
        init_udp_wrong_length,
        init_udp_wrong_header,
        init_udp_listener_fail,
        init_udp_timer_fail,

        proceed_msg_send,
        proceed_disconnect,
        proceed_recv_fail,
        proceed_send_fail,
        proceed_udp_wrong_header,
        proceed_udp_upd_recv,
        proceed_udp_timeout
    };

    struct session_result
    {
        session_status status;
        int err;
    };
}