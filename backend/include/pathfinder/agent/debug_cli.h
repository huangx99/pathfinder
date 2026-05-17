#pragma once

#include "pathfinder/agent/debug_export.h"
#include "pathfinder/agent/debug_report.h"
#include "pathfinder/agent/local_export.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>
#include <cstddef>

namespace pathfinder::agent {

// --- CLI Enums ---

enum class AgentDebugCliCommand {
    Unknown,
    Help,
    Export
};

std::string toString(AgentDebugCliCommand command);
pathfinder::foundation::Result<AgentDebugCliCommand> agentDebugCliCommandFromString(const std::string& str);

enum class AgentDebugCliFixture {
    Unknown,
    UnknownFruit,
    NoDecision,
    PublicSafe
};

std::string toString(AgentDebugCliFixture fixture);
pathfinder::foundation::Result<AgentDebugCliFixture> agentDebugCliFixtureFromString(const std::string& str);

enum class AgentDebugCliFormat {
    Unknown,
    Text,
    Markdown,
    ProtocolText
};

std::string toString(AgentDebugCliFormat format);
pathfinder::foundation::Result<AgentDebugCliFormat> agentDebugCliFormatFromString(const std::string& str);

enum class AgentDebugCliExitCode {
    Success = 0,
    InvalidArguments = 2,
    BuildFailed = 3,
    WriteFailed = 4,
    VerificationFailed = 5,
    InternalError = 10
};

int toInt(AgentDebugCliExitCode code);

// --- Data Contracts ---

struct AgentDebugCliOptions {
    AgentDebugCliCommand command = AgentDebugCliCommand::Unknown;
    AgentDebugCliFixture fixture = AgentDebugCliFixture::Unknown;
    AgentDebugCliFormat format = AgentDebugCliFormat::Markdown;
    std::string output_dir;
    std::string base_name = "agent_debug";
    bool dry_run = false;
    bool overwrite = false;
    bool help = false;
    bool scan_content = true;
    size_t max_items_per_chunk = 50;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentDebugCliResult {
    AgentDebugCliExitCode exit_code = AgentDebugCliExitCode::InternalError;
    std::string message_key;
    std::string summary_text;
    size_t planned_file_count = 0;
    size_t written_file_count = 0;
    std::string output_dir;
    std::vector<std::string> output_files;
    std::vector<std::string> warning_keys;
    std::vector<std::string> error_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentDebugCliParseResult {
    AgentDebugCliOptions options;
    AgentDebugCliResult result;
    bool should_execute = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- Fixture Bundle ---

struct AgentDebugFixtureBundle {
    AgentDebugReport report;
    AgentDiagnosticsSummary diagnostics;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- Services ---

class AgentDebugCliParser {
public:
    pathfinder::foundation::Result<AgentDebugCliParseResult> parse(
        int argc,
        const char* const argv[]) const;
};

class AgentDebugFixtureFactory {
public:
    pathfinder::foundation::Result<AgentDebugFixtureBundle> build(
        AgentDebugCliFixture fixture) const;
};

class AgentDebugCliRunner {
public:
    pathfinder::foundation::Result<AgentDebugCliResult> run(
        const AgentDebugCliOptions& options) const;
};

// --- Format conversion helper ---

AgentExportFormat toExportFormat(AgentDebugCliFormat cli_format);

} // namespace pathfinder::agent
