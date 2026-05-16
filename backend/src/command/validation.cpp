#include "pathfinder/command/validation.h"

namespace pathfinder::command {

std::string toString(CommandValidationSeverity severity) {
    switch (severity) {
        case CommandValidationSeverity::Warning: return "warning";
        case CommandValidationSeverity::Error:   return "error";
        default: return "unknown";
    }
}

void CommandValidationReport::addIssue(CommandValidationIssue issue) {
    issues_.push_back(std::move(issue));
}

bool CommandValidationReport::hasErrors() const {
    for (const auto& issue : issues_) {
        if (issue.severity == CommandValidationSeverity::Error) {
            return true;
        }
    }
    return false;
}

size_t CommandValidationReport::issueCount() const {
    return issues_.size();
}

const std::vector<CommandValidationIssue>& CommandValidationReport::issues() const {
    return issues_;
}

CommandValidationReport validateCommandEnvelopeBasic(const CommandEnvelope& envelope) {
    CommandValidationReport report;

    auto result = envelope.validateBasic();
    if (result.is_error()) {
        for (const auto& err : result.errors()) {
            report.addIssue(CommandValidationIssue{
                CommandValidationSeverity::Error,
                err.code,
                err.message_key,
                err.field_path.value_or("")
            });
        }
    }

    return report;
}

} // namespace pathfinder::command
