#pragma once

#include <stdexcept>
#include <string>

namespace json_fragments {

// Base class for all json_fragments exceptions
class JsonFragmentsError : public std::runtime_error {
public:
    explicit JsonFragmentsError(const std::string& message)
        : std::runtime_error(message) {}
};

// Thrown when a circular dependency is detected
class CircularDependencyError : public JsonFragmentsError {
public:
    explicit CircularDependencyError(const std::string& cycle)
        : JsonFragmentsError("Circular dependency detected: " + cycle) {}
};

// Thrown when a referenced fragment is not found
class FragmentNotFoundError : public JsonFragmentsError {
public:
    explicit FragmentNotFoundError(const std::string& fragment_name)
        : JsonFragmentsError("Fragment not found: " + fragment_name) {}
};

// Thrown when attempting to use a non-string value as an object key
class InvalidKeyError : public JsonFragmentsError {
public:
    explicit InvalidKeyError(const std::string& key)
        : JsonFragmentsError(
            "Fragment used as key must resolve to a string: " + key) {}
};

} // namespace json_fragments
