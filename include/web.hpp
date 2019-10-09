#ifndef GAMEBOT_WEB_HPP
#define GAMEBOT_WEB_HPP

#include "nlohmann/json.hpp"

namespace web
{

using json = nlohmann::json;

json get_bot_socket();

} // namespace web

#endif // GAMEBOT_WEB_HPP
