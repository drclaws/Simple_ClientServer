#include "message.h"

#include <string>
#include <cstring>

#include <simple_lib/common.h>

namespace simpleApp
{
    ssize_t proceedMsg(uint8_t* in, ssize_t len, uint8_t* out)
    {
        // TODO replace this with required logic
        const msg_headers buffHeader = msg_headers::server_msg;
        const char* msg = "DELIVERED";
        const ssize_t size = sizeof("DELIVERED");

        std::memcpy(out, &buffHeader, sizeof(buffHeader));
        std::memcpy(out + static_cast<ptrdiff_t>(sizeof(buffHeader)), msg, size);

        return sizeof(buffHeader) + size;
    }
}
