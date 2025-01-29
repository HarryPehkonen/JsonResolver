#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_container_properties.hpp>
#include <catch2/catch_approx.hpp> 
#include "json_fragments/json_resolver.hpp"
#include "json_fragments/exceptions.hpp"

using json = nlohmann::json;
using namespace json_fragments;

SCENARIO("JsonResolver handles basic fragment resolution", "[resolver]") {
    GIVEN("A JsonResolver and some simple fragments") {
        JsonResolver resolver;
        std::map<std::string, json> fragments;

        WHEN("resolving a fragment with no dependencies") {
            fragments["simple"] = {
                {"name", "Alice"},
                {"age", 30}
            };

            THEN("it returns the fragment as-is") {
                auto result = resolver.resolve(fragments, "simple");
                REQUIRE(result["name"] == "Alice");
                REQUIRE(result["age"] == 30);
            }
        }

        WHEN("resolving a fragment with a string reference") {
            fragments["name"] = "Bob";
            fragments["greeting"] = {
                {"message", "Hello, [name]!"}
            };

            THEN("it replaces the reference") {
                auto result = resolver.resolve(fragments, "greeting");
                REQUIRE(result["message"] == "Hello, Bob!");
            }
        }
    }
}

SCENARIO("JsonResolver preserves types during substitution", "[resolver]") {
    GIVEN("A JsonResolver and fragments with different types") {
        JsonResolver resolver;
        std::map<std::string, json> fragments;

        fragments["number"] = 42;
        fragments["float"] = 3.14;
        fragments["boolean"] = true;
        fragments["null_value"] = nullptr;

        WHEN("using these fragments in substitutions") {
            fragments["container"] = {
                {"int_value", "[number]"},
                {"float_value", "[float]"},
                {"bool_value", "[boolean]"},
                {"null_field", "[null_value]"}
            };

            THEN("the types are preserved") {
                auto result = resolver.resolve(fragments, "container");
                
                REQUIRE(result["int_value"].is_number_integer());
                REQUIRE(result["int_value"] == 42);
                
                REQUIRE(result["float_value"].is_number_float());
                REQUIRE(result["float_value"] == Catch::Approx(3.14));
                
                REQUIRE(result["bool_value"].is_boolean());
                REQUIRE(result["bool_value"] == true);
                
                REQUIRE(result["null_field"].is_null());
            }
        }
    }
}

SCENARIO("JsonResolver handles dynamic keys", "[resolver]") {
    GIVEN("A JsonResolver and fragments for LLM-style parameters") {
        JsonResolver resolver;
        std::map<std::string, json> fragments;

        fragments["param_name"] = "temperature";
        fragments["param_value"] = 0.7;

        WHEN("using a fragment as an object key") {
            fragments["tool_call"] = {
                {"type", "function"},
                {"[param_name]", "[param_value]"}
            };

            THEN("both key and value are properly resolved") {
                auto result = resolver.resolve(fragments, "tool_call");
                
                REQUIRE(result.contains("temperature"));
                REQUIRE(result["temperature"].is_number_float());
                REQUIRE(result["temperature"] == Catch::Approx(0.7));
                REQUIRE(result["type"] == "function");
            }
        }
    }
}

SCENARIO("JsonResolver detects circular dependencies", "[resolver]") {
    GIVEN("A JsonResolver and fragments with a circular reference") {
        JsonResolver resolver;
        std::map<std::string, json> fragments;

        fragments["A"] = {"ref", "[B]"};
        fragments["B"] = {"ref", "[C]"};
        fragments["C"] = {"ref", "[A]"};

        WHEN("attempting to resolve the circular reference") {
            THEN("it throws an exception with the cycle information") {
                REQUIRE_THROWS_AS(
                    resolver.resolve(fragments, "A"),
                    CircularDependencyError
                );
                /// REQUIRE_THROWS_WITH(
                ///     resolver.resolve(fragments, "A"),
                ///     Catch::Matchers::Contains("Circular dependency detected: A -> B -> C -> A")
                /// );
            }
        }
    }
}

SCENARIO("JsonResolver handles complex nested structures", "[resolver]") {
    GIVEN("A JsonResolver and fragments with nested references") {
        JsonResolver resolver;
        std::map<std::string, json> fragments;

        fragments["user"] = {
            {"id", 123},
            {"name", "Alice"}
        };

        fragments["metadata"] = {
            {"timestamp", "2024-01-29"},
            {"version", "1.0"}
        };

        fragments["message"] = {
            {"content", "Hello!"},
            {"user", "[user]"},
            {"meta", "[metadata]"}
        };

        WHEN("resolving the nested structure") {
            auto result = resolver.resolve(fragments, "message");

            THEN("all nested references are resolved") {
                REQUIRE(result["content"] == "Hello!");
                REQUIRE(result["user"]["id"] == 123);
                REQUIRE(result["user"]["name"] == "Alice");
                REQUIRE(result["meta"]["timestamp"] == "2024-01-29");
                REQUIRE(result["meta"]["version"] == "1.0");
            }
        }
    }
}

SCENARIO("JsonResolver handles array processing", "[resolver]") {
    GIVEN("A JsonResolver and fragments with arrays") {
        JsonResolver resolver;
        std::map<std::string, json> fragments;

        fragments["numbers"] = json::array({1, 2, 3});
        fragments["item"] = "test";

        WHEN("using fragments in arrays") {
            fragments["container"] = {
                {"direct_array", "[numbers]"},
                {"array_with_refs", json::array({"[item]", "[item]"})}
            };

            THEN("arrays are properly handled") {
                auto result = resolver.resolve(fragments, "container");
                
                REQUIRE(result["direct_array"].is_array());
                REQUIRE(result["direct_array"].size() == 3);
                REQUIRE(result["direct_array"][0] == 1);
                REQUIRE(result["direct_array"][1] == 2);
                REQUIRE(result["direct_array"][2] == 3);
                
                REQUIRE(result["array_with_refs"].is_array());
                REQUIRE(result["array_with_refs"].size() == 2);
                REQUIRE(result["array_with_refs"][0] == "test");
                REQUIRE(result["array_with_refs"][1] == "test");
            }
        }
    }
}

SCENARIO("JsonResolver handles error cases", "[resolver]") {
    GIVEN("A JsonResolver instance") {
        JsonResolver resolver;
        std::map<std::string, json> fragments;

        WHEN("resolving a non-existent fragment") {
            THEN("it throws an appropriate exception") {
                REQUIRE_THROWS_AS(
                    resolver.resolve(fragments, "missing"),
                    FragmentNotFoundError
                );
            }
        }

        WHEN("using a non-string fragment as a key") {
            fragments["number"] = 42;
            fragments["invalid"] = {
                {"[number]", "value"}
            };

            THEN("it throws an appropriate exception") {
                REQUIRE_THROWS_AS(
                    resolver.resolve(fragments, "invalid"),
                    InvalidKeyError
                );
            }
        }
    }
}

SCENARIO("JsonResolver handles LLM-style tool calls", "[resolver]") {
    GIVEN("A JsonResolver with tool call fragments") {
        JsonResolver resolver;
        std::map<std::string, json> fragments;

        fragments["function_name"] = "set_temperature";
        fragments["param_name"] = "temperature";
        fragments["param_value"] = 0.7;
        fragments["param_name2"] = "top_p";
        fragments["param_value2"] = 0.95;

        WHEN("creating a tool call with multiple parameters") {
            fragments["tool_call"] = {
                {"type", "function"},
                {"function", "[function_name]"},
                {"[param_name]", "[param_value]"},
                {"[param_name2]", "[param_value2]"}
            };

            THEN("the tool call is properly constructed") {
                auto result = resolver.resolve(fragments, "tool_call");
                
                REQUIRE(result["type"] == "function");
                REQUIRE(result["function"] == "set_temperature");
                REQUIRE(result["temperature"] == Catch::Approx(0.7));
                REQUIRE(result["top_p"] == Catch::Approx(0.95));
            }
        }
    }
}
