#include "bot.hpp"
#include "debug.hpp"
#include "utils.hpp"
#include "web.hpp"

qb::Bot::Bot(const Flag flag)
{
    using namespace qb;
    if (bitwise_and<int>(flag, Flag::LazyInit)) return;
    start();
}

void qb::Bot::start()
{
    // Step 01 - Make API call to Discord /gateway/bot/ to get a WebSocket URL
    // This is something we might need to do intermittently, so we call a function to do it.
    auto socket_info = web::get_bot_socket();
    qb::print(socket_info.dump(4));
}
