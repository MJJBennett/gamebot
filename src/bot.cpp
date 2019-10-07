#include "bot.hpp"
#include "web.hpp"

qb::Bot::Bot()
{
    using namespace qb;
    // Step 01 - Make API call to Discord /gateway/bot/ to get a WebSocket URL
    // This is something we might need to do intermittently, so we call a function to do it.
    auto socket_info = web::get_bot_socket();
}
