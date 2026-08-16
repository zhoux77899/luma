cmake_minimum_required(VERSION 3.16)
project(luma-sdl-preview LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

set(LUMA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

file(GLOB_RECURSE LUMA_SOURCES CONFIGURE_DEPENDS "${LUMA_ROOT}/src/luma/*.cpp")
list(FILTER LUMA_SOURCES EXCLUDE REGEX "platform[/\\]cardputer")

add_executable(luma-sdl-preview
    main.cpp
    sdl-display-adapter.cpp
    sdl-input-adapter.cpp
    ${LUMA_SOURCES}
)

target_include_directories(luma-sdl-preview PRIVATE
    "${LUMA_ROOT}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(luma-sdl-preview PRIVATE SDL2::SDL2)
if(TARGET SDL2::SDL2main)
    target_link_libraries(luma-sdl-preview PRIVATE SDL2::SDL2main)
endif()
