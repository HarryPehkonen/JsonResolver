add_executable(json_fragments_tests
    test_json_resolver.cpp
)

target_link_libraries(json_fragments_tests
    PRIVATE
        json_fragments
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(json_fragments_tests)
