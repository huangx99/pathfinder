#include "pathfinder/config/loader.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace pathfinder::config {

bool ConfigLoader::isPathSafe(const std::string& relative_path) {
    // Reject empty paths
    if (relative_path.empty()) {
        return false;
    }

    // Reject absolute paths
    if (!relative_path.empty() && relative_path[0] == '/') {
        return false;
    }

    // Reject paths containing ..
    if (relative_path.find("..") != std::string::npos) {
        return false;
    }

    return true;
}

ConfigLoadResult ConfigLoader::loadFiles(const ConfigLoadRequest& request) {
    ConfigLoadResult result;

    for (const auto& relative_path : request.relative_files) {
        // Check path safety
        if (!isPathSafe(relative_path)) {
            result.validation_report.addIssue(ConfigValidationIssue{
                ConfigValidationSeverity::Error,
                pathfinder::foundation::ErrorCode::validation_config_invalid,
                "Unsafe file path: " + relative_path,
                relative_path
            });
            continue;
        }

        // Build full path
        std::string full_path = request.root_path;
        if (!full_path.empty() && full_path.back() != '/') {
            full_path += '/';
        }
        full_path += relative_path;

        // Try to read the file
        std::ifstream file(full_path);
        if (!file.is_open()) {
            result.validation_report.addIssue(ConfigValidationIssue{
                ConfigValidationSeverity::Error,
                pathfinder::foundation::ErrorCode::storage_read_failed,
                "Cannot open file: " + full_path,
                relative_path
            });
            continue;
        }

        // Read content
        std::ostringstream content_stream;
        content_stream << file.rdbuf();
        std::string content = content_stream.str();

        // Check for empty file
        if (content.empty()) {
            result.validation_report.addIssue(ConfigValidationIssue{
                ConfigValidationSeverity::Warning,
                pathfinder::foundation::ErrorCode::validation_config_invalid,
                "Empty config file: " + relative_path,
                relative_path
            });
        }

        // Compute content hash
        auto content_hash = pathfinder::foundation::HashValue::fromString(content);

        // Add to result
        result.files.push_back(LoadedConfigFile{
            relative_path,
            std::move(content),
            content_hash
        });
    }

    return result;
}

} // namespace pathfinder::config
