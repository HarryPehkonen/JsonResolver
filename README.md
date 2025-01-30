# JsonResolver

A powerful C++ library for resolving JSON fragments with references, enabling dynamic JSON template resolution with robust dependency management.

## Features

- Fragment resolution with customizable delimiters (default: `[fragment_name]`)
- Dynamic object keys through fragment resolution
- Automatic circular dependency detection and prevention
- Flexible strategies for handling missing fragments
- Type preservation during substitution
- Rich error handling with detailed path information
- Support for nested fragment references
- String template interpolation

## Integration

### Using FetchContent

Add to your CMakeLists.txt:

```cmake
include(FetchContent)

FetchContent_Declare(
    json_fragments
    GIT_REPOSITORY https://github.com/yourusername/json_fragments
    GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(json_fragments)

target_link_libraries(your_target PRIVATE json_fragments::json_fragments)
```

### Using find_package

If json_fragments is installed on your system:

```cmake
find_package(json_fragments 0.1.0 REQUIRED)
target_link_libraries(your_target PRIVATE json_fragments::json_fragments)
```

## Usage Examples

### Basic Fragment Resolution

The simplest use case involves direct fragment references:

```cpp
#include <json_fragments/json_resolver.hpp>
#include <map>

using json = nlohmann::json;
using namespace json_fragments;

int main() {
    JsonResolver resolver;
    
    std::map<std::string, json> fragments;
    fragments["user"] = "Alice";
    fragments["greeting"] = {
        {"message", "Hello, [user]!"}
    };
    
    json result = resolver.resolve(fragments, "greeting");
    // result = {"message": "Hello, Alice!"}
}
```

### Loading Fragments from JSON Files

You can load fragments from JSON files:

```cpp
#include <json_fragments/json_resolver.hpp>
#include <fstream>

int main() {
    JsonResolver resolver;
    
    // Read fragments from file
    std::ifstream file("fragments.json");
    json fragments_json = json::parse(file);
    
    // Convert to fragment map
    std::map<std::string, json> fragments;
    for (const auto& [key, value] : fragments_json.items()) {
        fragments[key] = value;
    }
    
    json result = resolver.resolve(fragments, "main");
}
```

Example fragments.json:
```json
{
    "database": {
        "host": "localhost",
        "port": 5432
    },
    "api": {
        "endpoint": "http://[database.host]:[database.port]/api"
    },
    "main": {
        "config": {
            "api_url": "[api.endpoint]",
            "version": "1.0"
        }
    }
}
```

### Loading Fragments from String

You can also parse JSON directly from strings:

```cpp
std::string json_str = R"(
{
    "colors": {
        "primary": "#FF0000",
        "secondary": "#00FF00"
    },
    "theme": {
        "button": {
            "background": "[colors.primary]",
            "text": "#FFFFFF"
        }
    }
})";

std::map<std::string, json> fragments;
json parsed = json::parse(json_str);
for (const auto& [key, value] : parsed.items()) {
    fragments[key] = value;
}

json result = resolver.resolve(fragments, "theme");
```

### Dynamic Object Keys

JsonResolver supports using fragments as object keys:

```cpp
fragments["field_name"] = "status";
fragments["field_value"] = "active";
fragments["document"] = {
    {"id", 123},
    {"[field_name]", "[field_value]"}
};

json result = resolver.resolve(fragments, "document");
// result = {"id": 123, "status": "active"}
```

### Custom Configuration

You can customize the resolver's behavior:

```cpp
JsonResolverConfig config;

// Change fragment delimiters
config.delimiters.start = "{{";
config.delimiters.end = "}}";

// Set behavior for missing fragments
config.missing_fragment_behavior = 
    JsonResolverConfig::MissingFragmentBehavior::UseDefault;
config.default_value = "N/A";

JsonResolver resolver(config);
```

### Nested Fragment Resolution

Fragments can reference other fragments to any depth:

```cpp
fragments["user"] = {
    "name": "Alice",
    "role": "[role]"
};

fragments["role"] = {
    "title": "Admin",
    "permissions": "[permissions]"
};

fragments["permissions"] = ["read", "write", "execute"];

json result = resolver.resolve(fragments, "user");
// result = {
//     "name": "Alice",
//     "role": {
//         "title": "Admin",
//         "permissions": ["read", "write", "execute"]
//     }
// }
```

### Template String Interpolation

The resolver can handle multiple references within a single string:

```cpp
fragments["domain"] = "example.com";
fragments["port"] = "8080";
fragments["protocol"] = "https";

fragments["url"] = "[protocol]://[domain]:[port]/api";

json result = resolver.resolve(fragments, "url");
// result = "https://example.com:8080/api"
```

### Error Handling

The library provides detailed error information:

```cpp
try {
    json result = resolver.resolve(fragments, "missing_fragment");
} catch (const FragmentNotFoundError& e) {
    std::cerr << "Fragment not found: " << e.what() << '\n';
} catch (const CircularDependencyError& e) {
    std::cerr << "Circular dependency detected: " << e.what() << '\n';
} catch (const InvalidKeyError& e) {
    std::cerr << "Invalid key: " << e.what() << '\n';
} catch (const JsonFragmentsError& e) {
    std::cerr << "General error: " << e.what() << '\n';
}
```

## Building and Testing

Build the library:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Run tests:
```bash
ctest --output-on-failure
```

## Requirements

- C++17 or later
- CMake 3.15 or later
- nlohmann_json 3.11.2 or later

## License

This project is in the public domain - see the LICENSE file for details.
