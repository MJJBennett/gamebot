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

add_executable(QueueBotTests Test_Parse.cpp Test_FileIO.cpp Test_Utils.cpp Test_Component.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/parse.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/fileio.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/utils.cpp
)
target_include_directories(QueueBotTests PRIVATE "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>")
target_include_directories(QueueBotTests PRIVATE "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/deps/nlohmann_json/single_include>")

if (HAS_FS)
target_link_libraries(QueueBotTests gtest_main)
else()
target_link_libraries(QueueBotTests gtest_main stdc++fs)
endif(HAS_FS)
add_test(NAME Simple COMMAND QueueBotTests)

else()
message("Warning: Could not build tests. Reason: Google Test directory not found.")
endif()
