add_subdirectory(games)

target_sources(GameBot
    PRIVATE
        messages.cpp
        queue.cpp
        sentiment.cpp
    PUBLIC
        config.hpp
        messages.hpp
        queue.hpp
        sentiment.hpp
)
