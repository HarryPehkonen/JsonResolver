#pragma once

#include <map>
#include <set>
#include <string>
#include <nlohmann/json.hpp>
#include "fragment_nodes.hpp"

namespace json_fragments {

class JsonResolver {
public:
    explicit JsonResolver(JsonResolverConfig config = {});
    ~JsonResolver();

    // Main entry point - resolves a fragment and all its dependencies
    nlohmann::json resolve(
        const std::map<std::string, nlohmann::json>& fragments,
        const std::string& start_fragment
    );

private:
    JsonResolverConfig config_;
    EvaluationContext context_;
};

} // namespace json_fragments
