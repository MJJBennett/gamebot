SET(GOOGLETEST_DIR ${PROJECT_SOURCE_DIR}/deps/googletest)
if(IS_DIRECTORY ${GOOGLETEST_DIR})

project(QueueBotTests)
# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${GOOGLETEST_DIR}
                 EXCLUDE_FROM_ALL)

add_executable(QueueBotTests Test_Parse.cpp)
target_include_directories(QueueBotTests PRIVATE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
target_link_libraries(QueueBotTests gtest_main)
add_test(NAME Simple COMMAND QueueBotTests)

else()
message("Warning: Could not build tests. Reason: Google Test directory not found.")
endif()
