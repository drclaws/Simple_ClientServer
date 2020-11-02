#pragma once

namespace simpleApp
{
    enum class console_state
    {
        none,
        protocol_selection,
        address_input,
        
        connect_wait,
        msg_input,
        wait_result
    };
}