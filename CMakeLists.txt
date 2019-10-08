cmake_minimum_required(VERSION 3.3)
project(GameBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(JSON_BuildTests OFF CACHE INTERNAL "")

include_directories(include auth)
include_directories(/usr/local/opt/openssl/include)
set(OPENSSL_ROOT_DIR /usr/local/opt/openssl/include)

find_package(Boost 1.71 COMPONENTS program_options REQUIRED)
find_package(OpenSSL REQUIRED)
add_subdirectory(deps/nlohmann_json)

add_executable(GameBot src/bot.cpp src/auth.cpp src/utils.cpp src/debug.cpp include/web.hpp src/main.cpp include/flags.hpp include/bin_tools.hpp src/web.cpp)
target_link_libraries(GameBot PUBLIC OpenSSL::SSL)
target_link_libraries(GameBot PRIVATE Boost::program_options)
target_link_libraries(GameBot PRIVATE nlohmann_json::nlohmann_json)
