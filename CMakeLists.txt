cmake_minimum_required(VERSION 3.3)
project(GameBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(JSON_BuildTests OFF CACHE INTERNAL "")

include_directories(include auth)
include_directories(/usr/local/opt/openssl/include)

find_package(Boost 1.71 COMPONENTS program_options REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads)
add_subdirectory(deps/nlohmann_json)

include_directories(${Boost_INCLUDE_DIR})

try_compile(HAS_FS "${CMAKE_BINARY_DIR}/temp"
    "${CMAKE_SOURCE_DIR}/test/has_filesystem.cpp"
    CMAKE_FLAGS -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON)

add_executable(GameBot
    include/config.hpp
    src/fileio.cpp include/fileio.hpp
    src/messages.cpp include/messages.hpp
    src/parse.cpp include/parse.hpp
    src/utils.cpp include/utils.hpp
    src/sentiment.cpp include/sentiment.hpp
    src/queue.cpp include/queue.hpp
    include/debug.hpp
    src/wswrapper.cpp include/wswrapper.hpp
    src/web.cpp include/web.hpp
    src/bot.cpp include/bot.hpp
    src/main.cpp
)
target_link_libraries(GameBot PUBLIC ${CMAKE_THREAD_LIBS_INIT})
if (HAS_FS)
else()
target_link_libraries(GameBot PRIVATE stdc++fs)
endif(HAS_FS)
target_link_libraries(GameBot PUBLIC OpenSSL::SSL)
target_link_libraries(GameBot PRIVATE ${Boost_LIBRARIES})
target_link_libraries(GameBot PRIVATE nlohmann_json::nlohmann_json)

if(DEFINED QBTEST)
message(STATUS "Running with Testing set: ${QBTEST}")
enable_testing()
add_subdirectory(test)
endif()
