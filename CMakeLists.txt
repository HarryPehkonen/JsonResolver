cmake_minimum_required(VERSION 3.15)

# Define the project and version
project(json_fragments
    VERSION 0.1.0
    DESCRIPTION "A library for resolving JSON fragments with references"
    HOMEPAGE_URL "https://github.com/yourusername/json_fragments"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define options
option(JSON_FRAGMENTS_BUILD_TESTS "Build json_fragments tests" ON)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(FetchContent)

# Dependencies
# First try to find nlohmann_json
find_package(nlohmann_json 3.11.2 QUIET)

if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found. Fetching...")
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    FetchContent_MakeAvailable(json)
endif()

# Create the library target
add_library(json_fragments
    src/json_resolver.cpp
)
add_library(json_fragments::json_fragments ALIAS json_fragments)

# Set target properties
target_include_directories(json_fragments
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(json_fragments
    PUBLIC
        nlohmann_json::nlohmann_json
)

# Install rules
install(TARGETS json_fragments
    EXPORT json_fragments-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/json_fragments
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Generate and install package files
write_basic_package_version_file(
    "json_fragments-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/json_fragments-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/json_fragments-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/json_fragments
)

install(EXPORT json_fragments-targets
    FILE json_fragments-targets.cmake
    NAMESPACE json_fragments::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/json_fragments
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/json_fragments-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/json_fragments-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/json_fragments
)

# Tests
if(JSON_FRAGMENTS_BUILD_TESTS)
    # Set up testing
    enable_testing()
    
    # Find or fetch Catch2
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        message(STATUS "Catch2 not found. Fetching...")
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
        )
        FetchContent_MakeAvailable(Catch2)
        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    endif()

    add_subdirectory(tests)
endif()
