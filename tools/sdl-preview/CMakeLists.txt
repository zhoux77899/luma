cmake_minimum_required(VERSION 3.16)
project(luma-sdl-preview LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

get_filename_component(LUMA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
file(TO_CMAKE_PATH "${LUMA_ROOT}" LUMA_ROOT_POSIX)

file(GLOB_RECURSE LUMA_SOURCES CONFIGURE_DEPENDS "${LUMA_ROOT}/src/luma/*.cpp")
list(FILTER LUMA_SOURCES EXCLUDE REGEX "platform[/\\]cardputer")

set(PREVIEW_COMMON_SOURCES
    main.cpp
    preview-actions.cpp
    preview-host.cpp
    preview-report.cpp
    preview-screenshot.cpp
    sdl-audio-adapter.cpp
    sdl-display-adapter.cpp
    sdl-input-adapter.cpp
)

if(WIN32)
    list(APPEND PREVIEW_COMMON_SOURCES
        preview-clipboard-win.cpp
        preview-menu-win.cpp
        preview-save-dialog-win.cpp
    )
elseif(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    list(APPEND PREVIEW_COMMON_SOURCES
        preview-clipboard-macos.mm
        preview-menu-macos.mm
        preview-save-dialog-macos.mm
    )
else()
    list(APPEND PREVIEW_COMMON_SOURCES
        preview-clipboard-linux.cpp
        preview-menu-linux.cpp
        preview-save-dialog-linux.cpp
    )
endif()

add_executable(luma-sdl-preview
    ${PREVIEW_COMMON_SOURCES}
    ${LUMA_SOURCES}
)

target_include_directories(luma-sdl-preview PRIVATE
    "${LUMA_ROOT}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(luma-sdl-preview PRIVATE
    LUMA_BUILD_ENV="luma-sdl-preview"
    "LUMA_SOURCE_ROOT=\"${LUMA_ROOT_POSIX}\""
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

if(TARGET SDL2::SDL2main)
    target_link_libraries(luma-sdl-preview PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(luma-sdl-preview PRIVATE SDL2::SDL2)

if(WIN32)
    target_link_libraries(luma-sdl-preview PRIVATE comdlg32 user32 gdi32)
elseif(APPLE)
    target_link_libraries(luma-sdl-preview PRIVATE "-framework Cocoa")
else()
    find_package(X11 REQUIRED)
    target_link_libraries(luma-sdl-preview PRIVATE X11::X11)
endif()

if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
    add_custom_command(TARGET luma-sdl-preview POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:luma-sdl-preview>
            $<TARGET_FILE_DIR:luma-sdl-preview>
        COMMAND_EXPAND_LISTS
    )
endif()

add_executable(preview-scale-check
    preview-scale-check.cpp
    preview-screenshot.cpp
)
target_include_directories(preview-scale-check PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_definitions(preview-scale-check PRIVATE
    "LUMA_SOURCE_ROOT=\"${LUMA_ROOT_POSIX}\""
)
