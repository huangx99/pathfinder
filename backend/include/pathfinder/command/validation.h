#pragma once

#include "pathfinder/foundation/error.h"
#include "pathfinder/command/envelope.h"
#include <string>
#include <vector>

namespace pathfinder::command {

// Severity levels for command validation issues
enum class CommandValidationSeverity {
    Warning,
    Error
};

// Convert to string
std::string toString(CommandValidationSeverity severity);

// A single command validation issue
struct CommandValidationIssue {
    CommandValidationSeverity severity;
    pathfinder::foundation::ErrorCode code;
    std::string message;
    std::string field_path;
};

// Report collecting command validation issues
class CommandValidationReport {
public:
    CommandValidationReport() = default;

    // Add an issue
    void addIssue(CommandValidationIssue issue);

    // Check if there are any error issues
    bool hasErrors() const;

    // Get total issue count
    size_t issueCount() const;

    // Get all issues
    const std::vector<CommandValidationIssue>& issues() const;

private:
    std::vector<CommandValidationIssue> issues_;
};

// Validate a CommandEnvelope using basic validation
// This function calls envelope.validateBasic() and wraps the result
CommandValidationReport validateCommandEnvelopeBasic(const CommandEnvelope& envelope);

} // namespace pathfinder::command
