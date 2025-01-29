#include "json_fragments/json_resolver.hpp"
#include "json_fragments/exceptions.hpp"
#include <regex>

namespace json_fragments {

nlohmann::json JsonResolver::resolve(
    const std::map<std::string, nlohmann::json>& fragments,
    const std::string& start_fragment
) {
    std::set<std::string> processing_stack;
    return resolve_fragment(start_fragment, fragments, processing_stack);
}

nlohmann::json JsonResolver::resolve_fragment(
    const std::string& name,
    const std::map<std::string, nlohmann::json>& fragments,
    std::set<std::string>& processing_stack
) {
    // Check for circular dependencies
    if (processing_stack.count(name) > 0) {
        std::string cycle = name;
        for (const auto& fragment : processing_stack) {
            cycle += " -> " + fragment;
        }
        cycle += " -> " + name;
        throw CircularDependencyError(cycle);
    }

    // Find the fragment
    auto it = fragments.find(name);
    if (it == fragments.end()) {
        throw FragmentNotFoundError(name);
    }

    // Add to processing stack and process the fragment
    processing_stack.insert(name);
    auto result = process_value(it->second, fragments, processing_stack);
    processing_stack.erase(name);

    return result;
}

nlohmann::json JsonResolver::process_value(
    const nlohmann::json& value,
    const std::map<std::string, nlohmann::json>& fragments,
    std::set<std::string>& processing_stack
) {
    // Handle strings - check for fragment references
    if (value.is_string()) {
        const std::string& str = value;
        if (is_fragment_reference(str)) {
            // The entire string is a fragment reference
            return resolve_fragment(
                extract_fragment_name(str),
                fragments,
                processing_stack
            );
        } else {
            // Check for embedded fragment references
            static const std::regex ref_pattern("\\[(.*?)\\]");
            std::string result = str;
            std::smatch matches;
            std::string working_copy = str;
            
            // Find all fragment references and resolve them
            while (std::regex_search(working_copy, matches, ref_pattern)) {
                std::string fragment_name = matches[1].str();
                std::set<std::string> temp_stack = processing_stack;
                auto resolved = resolve_fragment(fragment_name, fragments, temp_stack);
                
                // Fragment references in strings must resolve to string values
                if (!resolved.is_string()) {
                    throw InvalidKeyError(fragment_name);
                }
                
                // Replace the reference with the resolved value
                std::string search = "[" + fragment_name + "]";
                size_t pos = result.find(search);
                if (pos != std::string::npos) {
                    result.replace(pos, search.length(), resolved.get<std::string>());
                }
                
                working_copy = matches.suffix();
            }
            return result;
        }
    }

    // Handle objects - need to process both keys and values
    if (value.is_object()) {
        nlohmann::json result;
        for (auto it = value.begin(); it != value.end(); ++it) {
            // Process the key if it's a fragment reference
            std::string resolved_key = it.key();
            if (is_fragment_reference(resolved_key)) {
                auto key_fragment = resolve_fragment(
                    extract_fragment_name(resolved_key),
                    fragments,
                    processing_stack
                );
                if (!key_fragment.is_string()) {
                    throw InvalidKeyError(resolved_key);
                }
                resolved_key = key_fragment.get<std::string>();
            }

            // Process the value
            result[resolved_key] = process_value(
                it.value(),
                fragments,
                processing_stack
            );
        }
        return result;
    }

    // Handle arrays
    if (value.is_array()) {
        nlohmann::json result = nlohmann::json::array();
        for (const auto& item : value) {
            result.push_back(
                process_value(item, fragments, processing_stack)
            );
        }
        return result;
    }

    // All other types (numbers, booleans, null) are returned as-is
    return value;
}

bool JsonResolver::is_fragment_reference(const std::string& str) const {
    return str.length() >= 2 && 
           str.front() == '[' && 
           str.back() == ']';
}

std::string JsonResolver::extract_fragment_name(
    const std::string& reference
) const {
    return reference.substr(1, reference.length() - 2);
}

} // namespace json_fragments
