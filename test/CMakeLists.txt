SET(GOOGLETEST_DIR ${CMAKE_SOURCE_DIR}/deps/googletest)
if(IS_DIRECTORY ${GOOGLETEST_DIR})

project(QueueBotTests)
# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${GOOGLETEST_DIR}
                 ${PROJECT_BINARY_DIR}/gtest
                 EXCLUDE_FROM_ALL)

add_executable(QueueBotTests Test_Parse.cpp Test_FileIO.cpp
    ${CMAKE_SOURCE_DIR}/src/parse.cpp
    ${CMAKE_SOURCE_DIR}/src/fileio.cpp
)
target_include_directories(QueueBotTests PRIVATE "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>")
target_include_directories(QueueBotTests PRIVATE "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/deps/nlohmann_json/single_include>")
target_link_libraries(QueueBotTests gtest_main stdc++fs)
add_test(NAME Simple COMMAND QueueBotTests)

else()
message("Warning: Could not build tests. Reason: Google Test directory not found.")
endif()
