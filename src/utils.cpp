#include "utils.hpp"

std::string qb::detail::get_client_id()
{
#include "client_id.hpp"
    // This is temporary, trust me
    return __client_id;
}

std::string qb::detail::get_bot_token()
{
#include "client_tok.hpp"
    // This is temporary, trust me
    return __client_tok;
}
