#pragma once

#include <map>
#include <set>
#include <string>
#include <nlohmann/json.hpp>

namespace json_fragments {

class JsonResolver {
public:
    // Main entry point - resolves a fragment and all its dependencies
    nlohmann::json resolve(
        const std::map<std::string, nlohmann::json>& fragments,
        const std::string& start_fragment
    );

private:
    // Internal helper to resolve a single fragment
    nlohmann::json resolve_fragment(
        const std::string& name,
        const std::map<std::string, nlohmann::json>& fragments,
        std::set<std::string>& processing_stack
    );

    // Process a JSON value, resolving any fragment references
    nlohmann::json process_value(
        const nlohmann::json& value,
        const std::map<std::string, nlohmann::json>& fragments,
        std::set<std::string>& processing_stack
    );

    // Helper to check if a string is a fragment reference
    bool is_fragment_reference(const std::string& str) const;
    
    // Helper to extract fragment name from reference
    std::string extract_fragment_name(const std::string& reference) const;
};

} // namespace json_fragments
