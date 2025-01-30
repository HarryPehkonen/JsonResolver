#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include "exceptions.hpp"

namespace json_fragments {

class DependencyTracker {
private:
    std::map<std::string, std::set<std::string>> dependencies_;
    std::set<std::string> currently_evaluating_;

public:
    // Record a dependency and check for cycles
    void add_dependency(const std::string& dependent, const std::string& dependency) {
        if (dependent.empty()) return;

        // Add the dependency
        dependencies_[dependent].insert(dependency);
        
        // Check for cycles after adding new dependency
        std::set<std::string> visited;
        std::vector<std::string> cycle_path;
        if (has_cycle_from(dependent, visited, cycle_path)) {
            throw CircularDependencyError(build_cycle_string(cycle_path));
        }
    }

    // Start evaluating a fragment
    void begin_evaluation(const std::string& fragment_name) {
        if (currently_evaluating_.find(fragment_name) != currently_evaluating_.end()) {
            throw CircularDependencyError(build_cycle_string(
                std::vector<std::string>(currently_evaluating_.begin(), 
                                       currently_evaluating_.end())));
        }
        currently_evaluating_.insert(fragment_name);
    }

    // End evaluating a fragment
    void end_evaluation(const std::string& fragment_name) {
        currently_evaluating_.erase(fragment_name);
    }

    // Access the dependency map (for debugging/testing)
    const auto& get_dependencies() const { return dependencies_; }

private:
    // Check for cycles in the dependency graph starting from a given node
    bool has_cycle_from(const std::string& start, 
                       std::set<std::string>& visited, 
                       std::vector<std::string>& path) const {
        if (std::find(path.begin(), path.end(), start) != path.end()) {
            if (path.empty() || path.back() != start) {
                path.push_back(start);
            }
            return true;
        }

        if (visited.find(start) != visited.end()) {
            return false;
        }

        visited.insert(start);
        path.push_back(start);

        auto deps_it = dependencies_.find(start);
        if (deps_it != dependencies_.end()) {
            for (const auto& dep : deps_it->second) {
                if (has_cycle_from(dep, visited, path)) {
                    return true;
                }
            }
        }

        path.pop_back();
        return false;
    }

    // Build a string representation of a cycle path
    static std::string build_cycle_string(const std::vector<std::string>& path) {
        std::string cycle;
        for (size_t i = 0; i < path.size(); ++i) {
            if (i > 0) cycle += " -> ";
            cycle += path[i];
        }
        if (!path.empty()) {
            cycle += " -> " + path.front();
        }
        return cycle;
    }
};

} // namespace json_fragments