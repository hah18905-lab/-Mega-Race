cmake_minimum_required(VERSION 3.16)
project(MegaRace C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# raylib 5.5 is downloaded automatically (needs internet on the first build)
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz)
FetchContent_MakeAvailable(raylib)

add_executable(MegaRace main.cpp)
target_link_libraries(MegaRace PRIVATE raylib)
if(WIN32)
    target_link_libraries(MegaRace PRIVATE winmm)
endif()
if(MSVC)
    target_compile_options(MegaRace PRIVATE /utf-8 /W3)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MegaRace)
endif()

# copy fonts and the music folder next to the executable
add_custom_command(TARGET MegaRace POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:MegaRace>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/music $<TARGET_FILE_DIR:MegaRace>/music)
