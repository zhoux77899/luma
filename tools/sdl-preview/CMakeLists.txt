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
    sdl-audio-adapter.cpp
    sdl-display-adapter.cpp
    sdl-input-adapter.cpp
    ${LUMA_SOURCES}
)

target_include_directories(luma-sdl-preview PRIVATE
    "${LUMA_ROOT}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(luma-sdl-preview PRIVATE
    LUMA_BUILD_ENV="luma-sdl-preview"
)
if(NOT DEFINED LUMA_VERSION)
    file(STRINGS "${LUMA_ROOT}/include/luma/version.h" _luma_version_line
         REGEX "^#define LUMA_VERSION \"")
    string(REGEX REPLACE "^#define LUMA_VERSION \"([^\"]+)\".*" "\\1"
           _luma_base_version "${_luma_version_line}")
    execute_process(
        COMMAND git rev-parse --short=7 HEAD
        WORKING_DIRECTORY "${LUMA_ROOT}"
        OUTPUT_VARIABLE _luma_git_sha
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_luma_git_sha)
        set(LUMA_VERSION "${_luma_base_version}.${_luma_git_sha}")
    else()
        set(LUMA_VERSION "${_luma_base_version}.dev")
    endif()
endif()
target_compile_definitions(luma-sdl-preview PRIVATE
    LUMA_VERSION="${LUMA_VERSION}"
)

if(APPLE)
    set_target_properties(luma-sdl-preview PROPERTIES
        BUILD_RPATH "@executable_path"
        INSTALL_RPATH "@executable_path"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
elseif(UNIX)
    set_target_properties(luma-sdl-preview PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

target_link_libraries(luma-sdl-preview PRIVATE SDL2::SDL2)
if(TARGET SDL2::SDL2main)
    target_link_libraries(luma-sdl-preview PRIVATE SDL2::SDL2main)
endif()

if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
    add_custom_command(TARGET luma-sdl-preview POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:luma-sdl-preview>
            $<TARGET_FILE_DIR:luma-sdl-preview>
        COMMAND_EXPAND_LISTS
    )
endif()
