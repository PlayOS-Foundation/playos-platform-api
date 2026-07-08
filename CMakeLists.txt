cmake_minimum_required(VERSION 3.20)
project(playos_platform_api VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── core library (engine-agnostic, no input backend) ──────────────────────
#
# Lifecycle + Capabilities only. Input is provided by a separately-linked
# backend that the consumer chooses.
add_library(playos-platform-api
    src/capabilities.cpp
    src/lifecycle.cpp
)
add_library(PlayOS::PlatformAPI ALIAS playos-platform-api)

target_include_directories(playos-platform-api PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(playos-platform-api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ── input backends (link-time choice) ─────────────────────────────────────
#
# The consumer picks one backend:
#   playos-input-raylib   wraps Raylib's gamepad API (recommended for PoC)
#   playos-input-xinput   wraps XInput (Windows only)
#   playos-input-evdev    evdev gamepad (Linux only)
#   playos-input-null     reports no input (placeholder)
#
# Each backend compiles input.cpp (the public Input API) alongside its own
# implementation of IInputBackend.

# Raylib backend — the recommended default for the vertical-slice PoC.
# Works on Windows and Linux; supports DualSense, Switch Pro, Xbox, etc.
# Only built when raylib is available (fetched by the consuming project).
if(TARGET raylib)
    add_library(playos-input-raylib
        src/input.cpp
        src/backends/raylib/raylib_input_backend.cpp)
    add_library(PlayOS::playos-input-raylib ALIAS playos-input-raylib)
    target_include_directories(playos-input-raylib PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(playos-input-raylib PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(playos-input-raylib PUBLIC raylib)
    target_compile_definitions(playos-input-raylib PRIVATE PLAYOS_INPUT_RAYLIB)
endif()

# XInput backend — Windows only, Xbox-compatible controllers.
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_library(playos-input-xinput
        src/input.cpp
        src/backends/windows/windows_input_backend.cpp)
    add_library(PlayOS::playos-input-xinput ALIAS playos-input-xinput)
    target_include_directories(playos-input-xinput PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(playos-input-xinput PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(playos-input-xinput PUBLIC Xinput)
    target_compile_definitions(playos-input-xinput PRIVATE PLAYOS_INPUT_XINPUT)
endif()

# evdev backend — Linux only, reads /dev/input/event* directly.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(playos-input-evdev
        src/input.cpp
        src/backends/linux/linux_input_backend.cpp)
    add_library(PlayOS::playos-input-evdev ALIAS playos-input-evdev)
    target_include_directories(playos-input-evdev PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(playos-input-evdev PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(playos-input-evdev PRIVATE PLAYOS_INPUT_EVDEV)
endif()

# Null backend — always available, reports no input.
add_library(playos-input-null
    src/input.cpp
    src/backends/null/null_input_backend.cpp)
add_library(PlayOS::playos-input-null ALIAS playos-input-null)
target_include_directories(playos-input-null PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_include_directories(playos-input-null PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(playos-input-null PRIVATE PLAYOS_INPUT_NULL)

# ── display backends (link-time choice) ───────────────────────────────────
#
# The consumer picks one backend:
#   playos-display-raylib  wraps Raylib's monitor API (recommended for PoC)
#   playos-display-null    returns safe defaults (placeholder)

# Raylib backend — recommended PoC default. Works on Windows and Linux.
# Only built when raylib is available (fetched by the consuming project).
if(TARGET raylib)
    add_library(playos-display-raylib
        src/display.cpp
        src/backends/raylib/raylib_display_backend.cpp)
    add_library(PlayOS::playos-display-raylib ALIAS playos-display-raylib)
    target_include_directories(playos-display-raylib PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(playos-display-raylib PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(playos-display-raylib PUBLIC raylib)
endif()

# Null backend — always available, returns 1280×720@60.
add_library(playos-display-null
    src/display.cpp
    src/backends/null/null_display_backend.cpp)
add_library(PlayOS::playos-display-null ALIAS playos-display-null)
target_include_directories(playos-display-null PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_include_directories(playos-display-null PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ── storage backends (link-time choice) ───────────────────────────────────
#
# The consumer picks one backend:
#   playos-storage-windows  %LOCALAPPDATA%/PlayOS (Windows only)
#   playos-storage-linux    XDG Base Directory paths (Linux only)
#   playos-storage-null     returns cwd (placeholder, always available)

# Windows backend — uses SHGetFolderPath(CSIDL_LOCAL_APPDATA).
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_library(playos-storage-windows
        src/storage.cpp
        src/backends/windows/windows_storage_backend.cpp)
    add_library(PlayOS::playos-storage-windows ALIAS playos-storage-windows)
    target_include_directories(playos-storage-windows PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(playos-storage-windows PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif()

# Linux backend — uses $XDG_DATA_HOME or ~/.local/share.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(playos-storage-linux
        src/storage.cpp
        src/backends/linux/linux_storage_backend.cpp)
    add_library(PlayOS::playos-storage-linux ALIAS playos-storage-linux)
    target_include_directories(playos-storage-linux PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(playos-storage-linux PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif()

# Null backend — always available, returns cwd/saves and cwd/config.
add_library(playos-storage-null
    src/storage.cpp
    src/backends/null/null_storage_backend.cpp)
add_library(PlayOS::playos-storage-null ALIAS playos-storage-null)
target_include_directories(playos-storage-null PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_include_directories(playos-storage-null PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ── audio backends (link-time choice) ─────────────────────────────────────
#
# The consumer picks one backend:
#   playos-audio-raylib  wraps Raylib's audio API (recommended for PoC)
#   playos-audio-null    no-op (placeholder)

# Raylib backend — recommended PoC default. Works on all Raylib platforms.
if(TARGET raylib)
    add_library(playos-audio-raylib
        src/audio.cpp
        src/backends/raylib/raylib_audio_backend.cpp)
    add_library(PlayOS::playos-audio-raylib ALIAS playos-audio-raylib)
    target_include_directories(playos-audio-raylib PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(playos-audio-raylib PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(playos-audio-raylib PUBLIC raylib)
endif()

# Null backend — always available, does nothing.
add_library(playos-audio-null
    src/audio.cpp
    src/backends/null/null_audio_backend.cpp)
add_library(PlayOS::playos-audio-null ALIAS playos-audio-null)
target_include_directories(playos-audio-null PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_include_directories(playos-audio-null PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ── touch backends (link-time choice) ─────────────────────────────────────
#
# The consumer picks one backend:
#   playos-touch-raylib  wraps Raylib's touch API (recommended for PoC)
#   playos-touch-null    reports no touch (placeholder)

# Raylib backend — recommended PoC default. Works on all Raylib platforms.
if(TARGET raylib)
    add_library(playos-touch-raylib
        src/touch.cpp
        src/backends/raylib/raylib_touch_backend.cpp)
    add_library(PlayOS::playos-touch-raylib ALIAS playos-touch-raylib)
    target_include_directories(playos-touch-raylib PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(playos-touch-raylib PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(playos-touch-raylib PUBLIC raylib)
endif()

# Null backend — always available, reports no touch points.
add_library(playos-touch-null
    src/touch.cpp
    src/backends/null/null_touch_backend.cpp)
add_library(PlayOS::playos-touch-null ALIAS playos-touch-null)
target_include_directories(playos-touch-null PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_include_directories(playos-touch-null PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
