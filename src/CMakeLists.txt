add_subdirectory(web)
add_subdirectory(utils)
add_subdirectory(components)

target_sources(GameBot
    PRIVATE
        bot.cpp
        bot.hpp
)
