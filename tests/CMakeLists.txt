# Create the test executable
add_executable(json_fragments_tests
    test_json_resolver.cpp
)

# Link against our library and Catch2
target_link_libraries(json_fragments_tests
    PRIVATE
        json_fragments
        Catch2::Catch2WithMain
)

# Include the Catch2 CMake scripts
include(Catch)

# Discover and register tests
catch_discover_tests(json_fragments_tests
    REPORTER XML
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/test-reports
    OUTPUT_PREFIX "test-"
    OUTPUT_SUFFIX .xml
)
