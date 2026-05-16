#pragma once

#include "pathfinder/foundation/hash.h"
#include "pathfinder/config/validation.h"
#include <string>
#include <vector>

namespace pathfinder::config {

// A single loaded config file
struct LoadedConfigFile {
    std::string path;
    std::string content;
    pathfinder::foundation::HashValue content_hash;
};

// Request to load config files
struct ConfigLoadRequest {
    std::string root_path;
    std::vector<std::string> relative_files;
};

// Result of loading config files
struct ConfigLoadResult {
    std::vector<LoadedConfigFile> files;
    ConfigValidationReport validation_report;
};

// ConfigLoader reads config files from disk
// P1 only: reads text, computes hash, basic file checks
// Does NOT parse JSON, does NOT build ConfigRegistry
class ConfigLoader {
public:
    // Load files from the request
    static ConfigLoadResult loadFiles(const ConfigLoadRequest& request);

private:
    // Check if a relative path is safe (no traversal, no absolute)
    static bool isPathSafe(const std::string& relative_path);
};

} // namespace pathfinder::config
