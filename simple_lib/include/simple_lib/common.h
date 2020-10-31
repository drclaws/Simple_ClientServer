#pragma once

#include <stdint.h>

namespace simpleApp
{
    const unsigned int WAIT_TIMEOUT_SEC = 15;

    enum class udp_header : uint8_t 
    {
        error =         0x80,
        sender_client = 0x40,
        sender_server = 0x20,

        connstart =     0x00,
        connup =        0x01,
        conndown =      0x02,
        message =       0x04,


        req_connstart =         sender_client | connstart,
        client_connup =         sender_client | connup,
        client_conndown =       sender_client | conndown,
        proceed_mgs =           sender_client | message,
        
        accept_connstart =      sender_server | connstart,
        server_connup =         sender_server | connup,
        server_conndown =       sender_server | conndown,
        proceeded_mgs =         sender_server | message,
        
        err_connstart =         accept_connstart | error,
        err_server_connup =     server_connup | error,
        err_proceeded_mgs =     proceeded_mgs | error
    };

    typedef int socket_t;

    int set_nonblock(int fd);

} // namespace simpleApp
