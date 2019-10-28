### QueueBot

QueueBot is an internet chat utility bot for [Discord](https://discord.gg). This project is only intended for personal use, but as the code interfaces with the Discord API on a relatively low level (using Boost::Beast and Boost::Asio for WebSocket and HTTP abstractions) the implementation may be useful when finding a starting point for implementing your own bot.

### Building QueueBot

QueueBot can be built by adding a `client_tok.hpp` file in `auth/` with the following contents: `const std::string __client_tok = "Your bot token here.";`

#### Dependencies

Run `scripts/grab-deps.sh` to clone `nlohmann/json`, a required dependency. You will have to install (and possibly build) Boost yourself. If running QueueBot leads to SSL errors, you may need to link to an updated version of OpenSSL, see `scripts/mac-build.sh` for an example CMake command to do this.
