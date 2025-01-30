#pragma once

#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace json_fragments {

// Forward declarations
class FragmentVisitor;
class FragmentNode;
struct JsonResolverConfig;

// Smart pointer alias for clarity
using FragmentNodePtr = std::unique_ptr<FragmentNode>;

// Base class for all nodes in our fragment tree
class FragmentNode {
public:
    virtual ~FragmentNode() = default;
    
    // Core evaluation method - converts node to final JSON value
    virtual nlohmann::json evaluate(
        const std::map<std::string, nlohmann::json>& fragments,
        const JsonResolverConfig& config
    ) const = 0;
    
    // Visitor pattern support
    virtual void accept(FragmentVisitor& visitor) = 0;
};

// Configuration for the resolver's behavior
struct JsonResolverConfig {
    // How to handle missing fragment references
    enum class MissingFragmentBehavior {
        Throw,              // Throw exception (default)
        LeaveUnresolved,    // Keep [fragment_name] as-is
        UseDefault,         // Use a default value
        Remove              // Remove the reference
    };
    
    struct Delimiters {
        std::string start = "[";
        std::string end = "]";
    };
    
    MissingFragmentBehavior missing_fragment_behavior = MissingFragmentBehavior::Throw;
    nlohmann::json default_value = nullptr;  // Used when behavior is UseDefault
    Delimiters delimiters;
};

// Visitor interface for fragment nodes
class FragmentVisitor {
public:
    virtual ~FragmentVisitor() = default;
    
    virtual void visit(class LiteralNode& node) = 0;
    virtual void visit(class ReferenceNode& node) = 0;
    virtual void visit(class StringTemplateNode& node) = 0;
    virtual void visit(class ObjectNode& node) = 0;
    virtual void visit(class ArrayNode& node) = 0;
};

// Class for tracking evaluation context and building error paths
class EvaluationContext {
    std::vector<std::string> path_;
    
public:
    // Add element to the path
    void push(const std::string& component) { 
        path_.push_back(component); 
    }
    
    // Remove last element from path
    void pop() { 
        if (!path_.empty()) path_.pop_back(); 
    }
    
    // Get the current path for dependency tracking
    const std::vector<std::string>& path() const { 
        return path_; 
    }
    
    // Get string representation of path for error messages
    std::string path_string() const {
        std::string result;
        for (const auto& component : path_) {
            result += "/" + component;
        }
        return result.empty() ? "/" : result;
    }
    
    // RAII helper for managing path components
    class ScopedComponent {
        EvaluationContext& context_;
    public:
        ScopedComponent(EvaluationContext& context, const std::string& component)
            : context_(context) {
            context_.push(component);
        }
        ~ScopedComponent() { 
            context_.pop(); 
        }
    };
};

} // namespace json_fragments
