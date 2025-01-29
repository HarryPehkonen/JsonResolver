#include "json_fragments/fragment_nodes.hpp"
#include "json_fragments/fragment_implementations.hpp"
#include "json_fragments/json_resolver.hpp"
#include "json_fragments/exceptions.hpp"

namespace json_fragments {

// The FragmentParser builds our AST by converting JSON values into appropriate nodes
class FragmentParser {
    EvaluationContext& context_;
    const JsonResolverConfig& config_;
    std::set<std::string>& dependency_stack_;  // Track fragment resolution path
    
    // Helper method to check if a string is a complete fragment reference
    bool is_complete_fragment_reference(const std::string& str) const {
        return str.length() >= (config_.delimiters.start.length() + config_.delimiters.end.length()) &&
               str.substr(0, config_.delimiters.start.length()) == config_.delimiters.start &&
               str.substr(str.length() - config_.delimiters.end.length()) == config_.delimiters.end;
    }
    
    // Helper to extract the fragment name from a reference
    std::string extract_fragment_name(const std::string& reference) const {
        return reference.substr(
            config_.delimiters.start.length(),
            reference.length() - config_.delimiters.start.length() - config_.delimiters.end.length()
        );
    }
    
public:
    FragmentParser(
        EvaluationContext& context, 
        const JsonResolverConfig& config,
        std::set<std::string>& dependency_stack
    )
        : context_(context)
        , config_(config)
        , dependency_stack_(dependency_stack) {}
    
    // Main entry point - converts JSON value into appropriate node type
    FragmentNodePtr parse(const nlohmann::json& input) {
        if (input.is_string()) {
            const std::string& str = input;
            if (is_complete_fragment_reference(str)) {
                std::string fragment_name = extract_fragment_name(str);
                
                // Check for circular dependencies
                if (dependency_stack_.find(fragment_name) != dependency_stack_.end()) {
                    // Build the cycle path for the error message
                    std::string cycle;
                    bool found_start = false;
                    for (const auto& dep : dependency_stack_) {
                        if (dep == fragment_name) {
                            found_start = true;
                        }
                        if (found_start) {
                            if (!cycle.empty()) cycle += " -> ";
                            cycle += dep;
                        }
                    }
                    cycle += " -> " + fragment_name;
                    throw CircularDependencyError(cycle);
                }

                // Add to dependency stack before creating node
                dependency_stack_.insert(fragment_name);
                return std::make_unique<ReferenceNode>(fragment_name, context_);
            }
            // Check if string contains any embedded fragment references
            if (str.find(config_.delimiters.start) != std::string::npos) {
                return std::make_unique<StringTemplateNode>(str, context_);
            }
            // Simple string literal
            return std::make_unique<LiteralNode>(str);
        }
        
        if (input.is_object()) {
            auto node = std::make_unique<ObjectNode>(context_);
            for (auto it = input.begin(); it != input.end(); ++it) {
                // Keys can be fragment references too
                FragmentNodePtr key_node;
                if (is_complete_fragment_reference(it.key())) {
                    key_node = std::make_unique<ReferenceNode>(
                        extract_fragment_name(it.key()),
                        context_
                    );
                } else {
                    key_node = std::make_unique<LiteralNode>(it.key());
                }
                    
                node->add_entry(
                    std::move(key_node),
                    parse(it.value())
                );
            }
            return node;
        }
        
        if (input.is_array()) {
            auto node = std::make_unique<ArrayNode>(context_);
            for (const auto& element : input) {
                node->add_element(parse(element));
            }
            return node;
        }
        
        // All other types (numbers, booleans, null) become literal nodes
        return std::make_unique<LiteralNode>(input);
    }
};

// Implementation of JsonResolver's member functions
JsonResolver::JsonResolver(JsonResolverConfig config)
    : config_(std::move(config)) {}

JsonResolver::~JsonResolver() = default;

nlohmann::json JsonResolver::resolve(
    const std::map<std::string, nlohmann::json>& fragments,
    const std::string& start_fragment
) {
    auto it = fragments.find(start_fragment);
    if (it == fragments.end()) {
        throw FragmentNotFoundError(start_fragment);
    }
    
    std::set<std::string> dependency_stack;
    dependency_stack.insert(start_fragment);
    
    context_.push(start_fragment);
    FragmentParser parser(context_, config_, dependency_stack);
    auto root_node = parser.parse(it->second);
    return root_node->evaluate(fragments, config_);
}

} // namespace json_fragments
