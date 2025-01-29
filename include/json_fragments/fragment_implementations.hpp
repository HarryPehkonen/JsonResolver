#pragma once

#include "fragment_nodes.hpp"
#include "exceptions.hpp"

namespace json_fragments {

// Represents a literal JSON value (number, boolean, null, or simple string)
class LiteralNode : public FragmentNode {
    nlohmann::json value_;
    
public:
    explicit LiteralNode(nlohmann::json value) : value_(std::move(value)) {}
    
    nlohmann::json evaluate(
        const std::map<std::string, nlohmann::json>&,
        const JsonResolverConfig&
    ) const override {
        return value_;
    }
    
    void accept(FragmentVisitor& visitor) override {
        visitor.visit(*this);
    }
    
    const nlohmann::json& value() const { return value_; }
};

// Represents a reference to another fragment
class ReferenceNode : public FragmentNode {
    std::string fragment_name_;
    EvaluationContext& context_;
    
public:
    ReferenceNode(std::string name, EvaluationContext& context) 
        : fragment_name_(std::move(name))
        , context_(context) {}
    
    nlohmann::json evaluate(
        const std::map<std::string, nlohmann::json>& fragments,
        const JsonResolverConfig& config
    ) const override {
        auto it = fragments.find(fragment_name_);
        if (it == fragments.end()) {
            switch (config.missing_fragment_behavior) {
                case JsonResolverConfig::MissingFragmentBehavior::Throw:
                    throw FragmentNotFoundError(fragment_name_);
                case JsonResolverConfig::MissingFragmentBehavior::LeaveUnresolved:
                    return config.delimiters.start + fragment_name_ + config.delimiters.end;
                case JsonResolverConfig::MissingFragmentBehavior::UseDefault:
                    return config.default_value;
                case JsonResolverConfig::MissingFragmentBehavior::Remove:
                    return "";
            }
        }
        
        // Add fragment to evaluation path for better error messages
        EvaluationContext::ScopedComponent path_component(context_, fragment_name_);
        return it->second;
    }
    
    void accept(FragmentVisitor& visitor) override {
        visitor.visit(*this);
    }
    
    const std::string& fragment_name() const { return fragment_name_; }
};

// Represents a string that might contain fragment references
class StringTemplateNode : public FragmentNode {
    std::string template_text_;
    EvaluationContext& context_;
    
public:
    StringTemplateNode(std::string text, EvaluationContext& context)
        : template_text_(std::move(text))
        , context_(context) {}
    
    nlohmann::json evaluate(
        const std::map<std::string, nlohmann::json>& fragments,
        const JsonResolverConfig& config
    ) const override {
        std::string result = template_text_;
        bool made_changes;
        
        do {
            made_changes = false;
            size_t pos = 0;
            
            while (pos < result.length()) {
                // Find innermost fragment reference
                size_t end_pos = result.find(config.delimiters.end, pos);
                if (end_pos == std::string::npos) break;
                
                size_t start_pos = result.rfind(config.delimiters.start, end_pos);
                if (start_pos == std::string::npos || start_pos < pos) {
                    pos = end_pos + 1;
                    continue;
                }
                
                // Extract and resolve the fragment
                std::string fragment_name = result.substr(
                    start_pos + config.delimiters.start.length(),
                    end_pos - (start_pos + config.delimiters.start.length())
                );
                
                try {
                    EvaluationContext::ScopedComponent path_component(
                        context_, 
                        "template:" + fragment_name
                    );
                    
                    auto it = fragments.find(fragment_name);
                    if (it == fragments.end()) {
                        switch (config.missing_fragment_behavior) {
                            case JsonResolverConfig::MissingFragmentBehavior::Throw:
                                throw FragmentNotFoundError(fragment_name);
                            case JsonResolverConfig::MissingFragmentBehavior::LeaveUnresolved:
                                pos = end_pos + 1;
                                continue;
                            case JsonResolverConfig::MissingFragmentBehavior::UseDefault:
                                if (!config.default_value.is_string()) {
                                    throw InvalidKeyError(
                                        "Default value for string template must be string"
                                    );
                                }
                                result.replace(
                                    start_pos,
                                    end_pos + config.delimiters.end.length() - start_pos,
                                    config.default_value.get<std::string>()
                                );
                                made_changes = true;
                                break;
                            case JsonResolverConfig::MissingFragmentBehavior::Remove:
                                result.replace(
                                    start_pos,
                                    end_pos + config.delimiters.end.length() - start_pos,
                                    ""
                                );
                                made_changes = true;
                                break;
                        }
                        continue;
                    }
                    
                    if (!it->second.is_string()) {
                        throw InvalidKeyError(
                            "Fragment in string template must resolve to string: " + 
                            fragment_name
                        );
                    }
                    
                    result.replace(
                        start_pos,
                        end_pos + config.delimiters.end.length() - start_pos,
                        it->second.get<std::string>()
                    );
                    made_changes = true;
                    
                } catch (const JsonFragmentsError& e) {
                    throw JsonFragmentsError(
                        std::string(e.what()) + " at " + context_.path_string()
                    );
                }
            }
        } while (made_changes);
        
        return result;
    }
    
    void accept(FragmentVisitor& visitor) override {
        visitor.visit(*this);
    }
    
    const std::string& template_text() const { return template_text_; }
};

// Represents a JSON object with possibly dynamic keys
class ObjectNode : public FragmentNode {
    std::vector<std::pair<FragmentNodePtr, FragmentNodePtr>> entries_;
    EvaluationContext& context_;
    
public:
    explicit ObjectNode(EvaluationContext& context) : context_(context) {}
    
    void add_entry(FragmentNodePtr key, FragmentNodePtr value) {
        entries_.emplace_back(std::move(key), std::move(value));
    }
    
    nlohmann::json evaluate(
        const std::map<std::string, nlohmann::json>& fragments,
        const JsonResolverConfig& config
    ) const override {
        nlohmann::json result;
        
        for (const auto& [key_node, value_node] : entries_) {
            auto key_result = key_node->evaluate(fragments, config);
            if (!key_result.is_string()) {
                throw InvalidKeyError("Object key must evaluate to string");
            }
            
            std::string key = key_result.get<std::string>();
            EvaluationContext::ScopedComponent path_component(context_, key);
            
            result[key] = value_node->evaluate(fragments, config);
        }
        
        return result;
    }
    
    void accept(FragmentVisitor& visitor) override {
        visitor.visit(*this);
    }
    
    const auto& entries() const { return entries_; }
};

// Represents a JSON array
class ArrayNode : public FragmentNode {
    std::vector<FragmentNodePtr> elements_;
    EvaluationContext& context_;
    
public:
    explicit ArrayNode(EvaluationContext& context) : context_(context) {}
    
    void add_element(FragmentNodePtr element) {
        elements_.push_back(std::move(element));
    }
    
    nlohmann::json evaluate(
        const std::map<std::string, nlohmann::json>& fragments,
        const JsonResolverConfig& config
    ) const override {
        nlohmann::json result = nlohmann::json::array();
        
        for (size_t i = 0; i < elements_.size(); ++i) {
            EvaluationContext::ScopedComponent path_component(
                context_, 
                std::to_string(i)
            );
            result.push_back(elements_[i]->evaluate(fragments, config));
        }
        
        return result;
    }
    
    void accept(FragmentVisitor& visitor) override {
        visitor.visit(*this);
    }
    
    const auto& elements() const { return elements_; }
};

} // namespace json_fragments
