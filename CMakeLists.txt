cmake_minimum_required(VERSION 3.20)
project(playos_platform_api VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(playos-platform-api
    src/capabilities.cpp
    src/input.cpp
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

# Select the platform backend. Windows is the primary SDK target; other
# platforms fall back to a null backend until a real backend is added.
if(WIN32)
    target_sources(playos-platform-api PRIVATE
        src/backends/windows/windows_input_backend.cpp)
    target_link_libraries(playos-platform-api PRIVATE Xinput)
    target_compile_definitions(playos-platform-api PRIVATE PLAYOS_BACKEND_WINDOWS)
else()
    target_sources(playos-platform-api PRIVATE
        src/backends/null/null_input_backend.cpp)
    target_compile_definitions(playos-platform-api PRIVATE PLAYOS_BACKEND_NULL)
endif()

option(PLAYOS_PLATFORM_API_BUILD_EXAMPLES "Build Platform API examples" OFF)
