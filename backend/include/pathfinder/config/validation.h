#pragma once

#include "pathfinder/foundation/error.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::config {

// Severity levels for config validation issues
enum class ConfigValidationSeverity {
    Info,
    Warning,
    Error,
    Fatal
};

// Convert ConfigValidationSeverity to string
std::string toString(ConfigValidationSeverity severity);

// A single config validation issue
struct ConfigValidationIssue {
    ConfigValidationSeverity severity;
    pathfinder::foundation::ErrorCode code;
    std::string message;
    std::optional<std::string> file_path;
    std::optional<std::string> json_path;
    int line = 0;    // 0 means unknown
    int column = 0;  // 0 means unknown
};

// Report collecting all config validation issues
class ConfigValidationReport {
public:
    ConfigValidationReport() = default;

    // Add an issue to the report
    void addIssue(ConfigValidationIssue issue);

    // Check if there are any error or fatal issues
    bool hasErrors() const;

    // Check if there are any fatal issues
    bool hasFatal() const;

    // Get total issue count
    size_t issueCount() const;

    // Get all issues
    const std::vector<ConfigValidationIssue>& issues() const;

private:
    std::vector<ConfigValidationIssue> issues_;
};

} // namespace pathfinder::config
