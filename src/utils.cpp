#include "utils.hpp"

std::string qb::detail::get_bot_token()
{
#include "client_tok.hpp"
    // Compile-time file I/O.
    // TODO - Don't do this.
    return __client_tok;
}
