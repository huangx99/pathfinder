#include "pathfinder/config/validation.h"

namespace pathfinder::config {

std::string toString(ConfigValidationSeverity severity) {
    switch (severity) {
        case ConfigValidationSeverity::Info:    return "info";
        case ConfigValidationSeverity::Warning: return "warning";
        case ConfigValidationSeverity::Error:   return "error";
        case ConfigValidationSeverity::Fatal:   return "fatal";
        default: return "unknown";
    }
}

void ConfigValidationReport::addIssue(ConfigValidationIssue issue) {
    issues_.push_back(std::move(issue));
}

bool ConfigValidationReport::hasErrors() const {
    for (const auto& issue : issues_) {
        if (issue.severity == ConfigValidationSeverity::Error ||
            issue.severity == ConfigValidationSeverity::Fatal) {
            return true;
        }
    }
    return false;
}

bool ConfigValidationReport::hasFatal() const {
    for (const auto& issue : issues_) {
        if (issue.severity == ConfigValidationSeverity::Fatal) {
            return true;
        }
    }
    return false;
}

size_t ConfigValidationReport::issueCount() const {
    return issues_.size();
}

const std::vector<ConfigValidationIssue>& ConfigValidationReport::issues() const {
    return issues_;
}

} // namespace pathfinder::config
