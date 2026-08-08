cmake_minimum_required(VERSION 3.18)
project(playos-platform-api
    VERSION 0.2.0
    DESCRIPTION "PlayOS public libplayos C ABI and engine adapters"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)

# Backend selection (overridden by Buildroot toolchain file)
set(PLAYOS_BACKEND "auto" CACHE STRING "Backend: auto, evdev, stub")

# ── Public headers ──────────────────────────────────────────────────────────
add_library(playos SHARED
    src/playos_system.c
    src/playos_lifecycle.c
    src/playos_input.c
    src/playos_display.c
    src/playos_storage.c
    src/playos_audio.c
    src/playos_power.c
    src/playos_logging.c
)

target_include_directories(playos
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(playos PROPERTIES
    SOVERSION 0
    VERSION   0.2.0
    OUTPUT_NAME playos
)

# Backend sources
if(PLAYOS_BACKEND STREQUAL "stub" OR PLAYOS_BACKEND STREQUAL "auto")
    target_sources(playos PRIVATE src/backends/backend_stub.c)
    target_compile_definitions(playos PRIVATE PLAYOS_BACKEND_STUB)
endif()

if(PLAYOS_BACKEND STREQUAL "evdev")
    target_sources(playos PRIVATE src/backends/backend_evdev.c)
    target_compile_definitions(playos PRIVATE PLAYOS_BACKEND_EVDEV)
endif()

# ── Install rules ───────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS playos
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/playos
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ── Tests ────────────────────────────────────────────────────────────────────
option(PLAYOS_BUILD_TESTS "Build unit tests" ON)

if(PLAYOS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
