#include "json_fragments/fragment_nodes.hpp"
#include "json_fragments/fragment_implementations.hpp"
#include "json_fragments/json_resolver.hpp"
#include "json_fragments/dependency_tracker.hpp"
#include "json_fragments/exceptions.hpp"

namespace json_fragments {

class FragmentParser {
private:
    EvaluationContext& context_;
    const JsonResolverConfig& config_;
    const std::map<std::string, nlohmann::json>& fragments_;
    DependencyTracker dependency_tracker_;

    // Helper class for RAII-style fragment evaluation
    class FragmentEvaluationGuard {
    private:
        DependencyTracker& tracker_;
        const std::string& fragment_name_;

    public:
        FragmentEvaluationGuard(DependencyTracker& tracker, const std::string& fragment_name)
            : tracker_(tracker)
            , fragment_name_(fragment_name) {
            tracker_.begin_evaluation(fragment_name_);
        }

        ~FragmentEvaluationGuard() {
            tracker_.end_evaluation(fragment_name_);
        }
    };
    
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

    // Evaluates a fragment and its dependencies
    void evaluate_fragment_dependencies(const std::string& fragment_name) {
        auto it = fragments_.find(fragment_name);
        if (it == fragments_.end()) {
            return;  // Fragment not found, skip evaluation
        }

        FragmentEvaluationGuard guard(dependency_tracker_, fragment_name);
        parse(it->second, fragment_name);
    }
    
public:
    FragmentParser(
        EvaluationContext& context,
        const JsonResolverConfig& config,
        const std::map<std::string, nlohmann::json>& fragments
    )
        : context_(context)
        , config_(config)
        , fragments_(fragments) {}
        
    // Main entry point - converts JSON value into appropriate node type
    FragmentNodePtr parse(const nlohmann::json& input, const std::string& current_fragment = "") {
        if (input.is_string()) {
            const std::string& str = input;
            if (is_complete_fragment_reference(str)) {
                std::string fragment_name = extract_fragment_name(str);
                
                if (!current_fragment.empty()) {
                    dependency_tracker_.add_dependency(current_fragment, fragment_name);
                    evaluate_fragment_dependencies(fragment_name);
                }

                return std::make_unique<ReferenceNode>(fragment_name, context_);
            }
            
            if (str.find(config_.delimiters.start) != std::string::npos) {
                return std::make_unique<StringTemplateNode>(str, context_);
            }
            
            return std::make_unique<LiteralNode>(str);
        }
        
        if (input.is_object()) {
            auto node = std::make_unique<ObjectNode>(context_);
            for (auto it = input.begin(); it != input.end(); ++it) {
                FragmentNodePtr key_node;
                if (is_complete_fragment_reference(it.key())) {
                    std::string fragment_name = extract_fragment_name(it.key());
                    if (!current_fragment.empty()) {
                        dependency_tracker_.add_dependency(current_fragment, fragment_name);
                        evaluate_fragment_dependencies(fragment_name);
                    }
                    key_node = std::make_unique<ReferenceNode>(fragment_name, context_);
                } else {
                    key_node = std::make_unique<LiteralNode>(it.key());
                }
                    
                node->add_entry(
                    std::move(key_node),
                    parse(it.value(), current_fragment)
                );
            }
            return node;
        }
        
        if (input.is_array()) {
            auto node = std::make_unique<ArrayNode>(context_);
            for (const auto& element : input) {
                node->add_element(parse(element, current_fragment));
            }
            return node;
        }
        
        return std::make_unique<LiteralNode>(input);
    }
    
    const auto& get_dependencies() const { return dependency_tracker_.get_dependencies(); }
};

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
    
    context_.push(start_fragment);
    FragmentParser parser(context_, config_, fragments);
    auto root_node = parser.parse(it->second, start_fragment);
    
    return root_node->evaluate(fragments, config_);
}

} // namespace json_fragments