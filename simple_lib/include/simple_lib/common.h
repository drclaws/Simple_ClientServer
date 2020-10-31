#pragma once

#include <stdint.h>

namespace simpleApp
{
    const unsigned int WAIT_TIMEOUT_SEC = 15;

    /// Message headers.
    /// TCP-sessions use only "Data proceeding" group.

    typedef uint8_t header_base_type;

    enum class msg_headers : header_base_type 
    {
        // Header flags
        error =         0x80,
        sender_client = 0x40,
        sender_server = 0x20,

        connstart =     0x01,
        connup =        0x02,
        conndown =      0x04,
        message =       0x08,

        timer =         0x10,

        // Init session headers
        req_connstart =         sender_client | connstart,
        accept_connstart =      sender_server | connstart,
        err_connstart =         sender_server | connstart | error,  // Send by listener

        // Session activity check
        client_connup =         sender_client | connup,
        server_connup =         sender_server | connup,

        // Session down info
        client_conndown =       sender_client | conndown,
        server_conndown =       sender_server | conndown,
        server_session_err =    sender_server | conndown | error,   // Connection down due server error
        
        // Timer info
        client_timeout =        sender_client | conndown | timer,
        server_timeout =        sender_server | conndown | timer,

        // Data proceeding (TCP possible)
        client_msg =            sender_client | message,
        server_msg =            sender_server | message,
        err_server_msg =        sender_server | message | error     // Error in client message
    };

    typedef int socket_t;

    int set_nonblock(int fd);

} // namespace simpleApp
