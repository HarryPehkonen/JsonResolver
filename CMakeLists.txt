cmake_minimum_required(VERSION 3.15)
project(json_fragments VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add JSON library
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(json)

# Add Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(Catch2)

# Library target
add_library(json_fragments
    src/json_resolver.cpp
)

target_include_directories(json_fragments
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(json_fragments
    PUBLIC
        nlohmann_json::nlohmann_json
)

# Tests
enable_testing()
add_subdirectory(tests)
