add_subdirectory(games)
add_subdirectory(tools)

target_sources(GameBot
    PRIVATE
        messages.cpp
        queue.cpp
        sentiment.cpp
        component.cpp
    PUBLIC
        component.hpp
        config.hpp
        messages.hpp
        queue.hpp
        sentiment.hpp
)
