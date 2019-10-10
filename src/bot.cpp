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

    // Step 05 - Get the socket URL from the JSON data and save it with explicit version/encoding
    const std::string socket_url = socket_info["url"];
    // This isn't particularly efficient but optimizing startup time doesn't seem like a priority

    // Step 06 - Start running bot core.

    // This requires a connection to the remote websocket server.
    // We will use our abstractions in web.hpp to acquire this for us.
    auto ws = web::acquire_websocket(socket_url);
    ws.validate();
    // For now, just disconnect again, as this is a work in progress.
    qb::print("Finishing bot execution...");
}
