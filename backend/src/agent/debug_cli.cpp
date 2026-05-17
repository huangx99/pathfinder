#include "pathfinder/agent/debug_cli.h"
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstring>

namespace pathfinder::agent {

// --- Enum toString / fromString ---

std::string toString(AgentDebugCliCommand command) {
    switch (command) {
        case AgentDebugCliCommand::Unknown: return "Unknown";
        case AgentDebugCliCommand::Help: return "Help";
        case AgentDebugCliCommand::Export: return "Export";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugCliCommand> agentDebugCliCommandFromString(const std::string& str) {
    if (str == "Unknown" || str == "unknown") return pathfinder::foundation::Result<AgentDebugCliCommand>::ok(AgentDebugCliCommand::Unknown);
    if (str == "Help" || str == "help") return pathfinder::foundation::Result<AgentDebugCliCommand>::ok(AgentDebugCliCommand::Help);
    if (str == "Export" || str == "export") return pathfinder::foundation::Result<AgentDebugCliCommand>::ok(AgentDebugCliCommand::Export);
    return pathfinder::foundation::Result<AgentDebugCliCommand>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "cli_command_from_string_failed",
            "Unknown AgentDebugCliCommand: " + str));
}

std::string toString(AgentDebugCliFixture fixture) {
    switch (fixture) {
        case AgentDebugCliFixture::Unknown: return "Unknown";
        case AgentDebugCliFixture::UnknownFruit: return "UnknownFruit";
        case AgentDebugCliFixture::NoDecision: return "NoDecision";
        case AgentDebugCliFixture::PublicSafe: return "PublicSafe";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugCliFixture> agentDebugCliFixtureFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDebugCliFixture>::ok(AgentDebugCliFixture::Unknown);
    if (str == "unknown_fruit" || str == "UnknownFruit") return pathfinder::foundation::Result<AgentDebugCliFixture>::ok(AgentDebugCliFixture::UnknownFruit);
    if (str == "no_decision" || str == "NoDecision") return pathfinder::foundation::Result<AgentDebugCliFixture>::ok(AgentDebugCliFixture::NoDecision);
    if (str == "public_safe" || str == "PublicSafe") return pathfinder::foundation::Result<AgentDebugCliFixture>::ok(AgentDebugCliFixture::PublicSafe);
    return pathfinder::foundation::Result<AgentDebugCliFixture>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "cli_fixture_from_string_failed",
            "Unknown AgentDebugCliFixture: " + str));
}

std::string toString(AgentDebugCliFormat format) {
    switch (format) {
        case AgentDebugCliFormat::Unknown: return "Unknown";
        case AgentDebugCliFormat::Text: return "Text";
        case AgentDebugCliFormat::Markdown: return "Markdown";
        case AgentDebugCliFormat::ProtocolText: return "ProtocolText";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugCliFormat> agentDebugCliFormatFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDebugCliFormat>::ok(AgentDebugCliFormat::Unknown);
    if (str == "text" || str == "Text") return pathfinder::foundation::Result<AgentDebugCliFormat>::ok(AgentDebugCliFormat::Text);
    if (str == "markdown" || str == "Markdown") return pathfinder::foundation::Result<AgentDebugCliFormat>::ok(AgentDebugCliFormat::Markdown);
    if (str == "protocol_text" || str == "ProtocolText") return pathfinder::foundation::Result<AgentDebugCliFormat>::ok(AgentDebugCliFormat::ProtocolText);
    return pathfinder::foundation::Result<AgentDebugCliFormat>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "cli_format_from_string_failed",
            "Unknown AgentDebugCliFormat: " + str));
}

int toInt(AgentDebugCliExitCode code) {
    return static_cast<int>(code);
}

// --- Format conversion ---

AgentExportFormat toExportFormat(AgentDebugCliFormat cli_format) {
    switch (cli_format) {
        case AgentDebugCliFormat::Text: return AgentExportFormat::PlainText;
        case AgentDebugCliFormat::Markdown: return AgentExportFormat::MarkdownLike;
        case AgentDebugCliFormat::ProtocolText: return AgentExportFormat::ProtocolText;
        default: return AgentExportFormat::Unknown;
    }
}

// --- Base name validation ---

static bool isValidBaseName(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

// --- AgentDebugCliOptions::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugCliOptions::validateBasic() const {
    if (help) {
        return pathfinder::foundation::Result<void>::ok();
    }
    if (command == AgentDebugCliCommand::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "cli_options_command_unknown"));
    }
    if (command == AgentDebugCliCommand::Export) {
        if (fixture == AgentDebugCliFixture::Unknown) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "cli_options_export_missing_fixture"));
        }
        if (output_dir.empty()) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "cli_options_export_missing_output_dir"));
        }
    }
    if (format == AgentDebugCliFormat::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "cli_options_format_unknown"));
    }
    if (!isValidBaseName(base_name)) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "cli_options_invalid_base_name"));
    }
    if (max_items_per_chunk == 0 || max_items_per_chunk > 1000) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "cli_options_max_items_per_chunk_out_of_range"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugCliResult::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugCliResult::validateBasic() const {
    if (summary_text.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "cli_result_empty_summary_text"));
    }
    if (exit_code == AgentDebugCliExitCode::Success && !error_keys.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "cli_result_success_with_error_keys"));
    }
    if (exit_code != AgentDebugCliExitCode::Success && error_keys.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "cli_result_failure_without_error_keys"));
    }
    for (const auto& f : output_files) {
        if (!f.empty() && f[0] == '/') {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "cli_result_absolute_path_in_output_files"));
        }
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugCliParseResult::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugCliParseResult::validateBasic() const {
    auto ov = options.validateBasic();
    if (ov.is_error()) return ov;
    auto rv = result.validateBasic();
    if (rv.is_error()) return rv;
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugFixtureBundle::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugFixtureBundle::validateBasic() const {
    auto rv = report.validateBasic();
    if (rv.is_error()) return rv;
    auto dv = diagnostics.validateBasic();
    if (dv.is_error()) return dv;
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugCliParser::parse ---

pathfinder::foundation::Result<AgentDebugCliParseResult> AgentDebugCliParser::parse(
    int argc,
    const char* const argv[]) const {

    AgentDebugCliParseResult parse_result;

    if (argc < 1) {
        parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
        parse_result.result.message_key = "cli_no_program_name";
        parse_result.result.summary_text = "Error: no program name";
        parse_result.result.error_keys.push_back("cli_no_program_name");
        return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
    }

    // Check for --help first (highest priority)
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0) {
            parse_result.options.help = true;
            parse_result.options.command = AgentDebugCliCommand::Help;
            parse_result.result.exit_code = AgentDebugCliExitCode::Success;
            parse_result.result.message_key = "cli_help";
            std::ostringstream oss;
            oss << "Usage: " << argv[0] << " [--help]\n";
            oss << "       " << argv[0] << " export --fixture <name> [--format <fmt>] --output-dir <dir>\n";
            oss << "              [--base-name <name>] [--dry-run] [--overwrite]\n";
            oss << "              [--max-items-per-chunk <n>]\n";
            oss << "\nFixtures: unknown_fruit, no_decision, public_safe\n";
            oss << "Formats:  text, markdown (default), protocol_text\n";
            parse_result.result.summary_text = oss.str();
            return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
        }
    }

    // Parse arguments
    AgentDebugCliOptions& opts = parse_result.options;
    int i = 1;

    // First arg might be the command
    if (i < argc && argv[i][0] != '-') {
        auto cmd_result = agentDebugCliCommandFromString(argv[i]);
        if (cmd_result.is_error() || cmd_result.value() == AgentDebugCliCommand::Unknown) {
            parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
            parse_result.result.message_key = "cli_unknown_command";
            parse_result.result.summary_text = std::string("Error: unknown command '") + argv[i] + "'";
            parse_result.result.error_keys.push_back("cli_unknown_command");
            return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
        }
        opts.command = cmd_result.value();
        ++i;
    }

    // Parse flags
    while (i < argc) {
        const char* arg = argv[i];

        if (std::strcmp(arg, "--help") == 0) {
            // Already handled above, but just in case
            opts.help = true;
            opts.command = AgentDebugCliCommand::Help;
            parse_result.result.exit_code = AgentDebugCliExitCode::Success;
            parse_result.result.message_key = "cli_help";
            std::ostringstream oss;
            oss << "Usage: " << argv[0] << " [--help]\n";
            oss << "       " << argv[0] << " export --fixture <name> [--format <fmt>] --output-dir <dir>\n";
            oss << "              [--base-name <name>] [--dry-run] [--overwrite]\n";
            oss << "              [--max-items-per-chunk <n>]\n";
            oss << "\nFixtures: unknown_fruit, no_decision, public_safe\n";
            oss << "Formats:  text, markdown (default), protocol_text\n";
            parse_result.result.summary_text = oss.str();
            return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
        }

        if (std::strcmp(arg, "--fixture") == 0) {
            if (i + 1 >= argc) {
                parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
                parse_result.result.message_key = "cli_fixture_missing_value";
                parse_result.result.summary_text = "Error: --fixture requires a value";
                parse_result.result.error_keys.push_back("cli_fixture_missing_value");
                return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
            }
            auto fix_result = agentDebugCliFixtureFromString(argv[++i]);
            if (fix_result.is_error() || fix_result.value() == AgentDebugCliFixture::Unknown) {
                parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
                parse_result.result.message_key = "cli_invalid_fixture";
                parse_result.result.summary_text = std::string("Error: invalid fixture '") + argv[i] + "'";
                parse_result.result.error_keys.push_back("cli_invalid_fixture");
                return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
            }
            opts.fixture = fix_result.value();
        } else if (std::strcmp(arg, "--format") == 0) {
            if (i + 1 >= argc) {
                parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
                parse_result.result.message_key = "cli_format_missing_value";
                parse_result.result.summary_text = "Error: --format requires a value";
                parse_result.result.error_keys.push_back("cli_format_missing_value");
                return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
            }
            auto fmt_result = agentDebugCliFormatFromString(argv[++i]);
            if (fmt_result.is_error() || fmt_result.value() == AgentDebugCliFormat::Unknown) {
                parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
                parse_result.result.message_key = "cli_invalid_format";
                parse_result.result.summary_text = std::string("Error: invalid format '") + argv[i] + "'";
                parse_result.result.error_keys.push_back("cli_invalid_format");
                return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
            }
            opts.format = fmt_result.value();
        } else if (std::strcmp(arg, "--output-dir") == 0) {
            if (i + 1 >= argc) {
                parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
                parse_result.result.message_key = "cli_output_dir_missing_value";
                parse_result.result.summary_text = "Error: --output-dir requires a value";
                parse_result.result.error_keys.push_back("cli_output_dir_missing_value");
                return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
            }
            opts.output_dir = argv[++i];
        } else if (std::strcmp(arg, "--base-name") == 0) {
            if (i + 1 >= argc) {
                parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
                parse_result.result.message_key = "cli_base_name_missing_value";
                parse_result.result.summary_text = "Error: --base-name requires a value";
                parse_result.result.error_keys.push_back("cli_base_name_missing_value");
                return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
            }
            opts.base_name = argv[++i];
        } else if (std::strcmp(arg, "--dry-run") == 0) {
            opts.dry_run = true;
        } else if (std::strcmp(arg, "--overwrite") == 0) {
            opts.overwrite = true;
        } else if (std::strcmp(arg, "--max-items-per-chunk") == 0) {
            if (i + 1 >= argc) {
                parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
                parse_result.result.message_key = "cli_max_items_missing_value";
                parse_result.result.summary_text = "Error: --max-items-per-chunk requires a value";
                parse_result.result.error_keys.push_back("cli_max_items_missing_value");
                return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
            }
            ++i;
            char* end = nullptr;
            long val = std::strtol(argv[i], &end, 10);
            if (end == argv[i] || *end != '\0' || val <= 0 || val > 1000) {
                parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
                parse_result.result.message_key = "cli_max_items_invalid";
                parse_result.result.summary_text = std::string("Error: invalid --max-items-per-chunk '") + argv[i] + "'";
                parse_result.result.error_keys.push_back("cli_max_items_invalid");
                return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
            }
            opts.max_items_per_chunk = static_cast<size_t>(val);
        } else if (std::strcmp(arg, "--json") == 0 ||
                   std::strcmp(arg, "--http") == 0 ||
                   std::strcmp(arg, "--websocket") == 0 ||
                   std::strcmp(arg, "--save") == 0 ||
                   std::strcmp(arg, "--load-save") == 0 ||
                   std::strcmp(arg, "--agent-log") == 0 ||
                   std::strcmp(arg, "--game-state") == 0 ||
                   std::strcmp(arg, "--rl") == 0 ||
                   std::strcmp(arg, "--llm") == 0) {
            parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
            parse_result.result.message_key = "cli_unsupported_argument";
            parse_result.result.summary_text = std::string("Error: unsupported argument '") + arg + "'";
            parse_result.result.error_keys.push_back("cli_unsupported_argument");
            return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
        } else if (arg[0] == '-') {
            // Unknown flag
            parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
            parse_result.result.message_key = "cli_unknown_argument";
            parse_result.result.summary_text = std::string("Error: unknown argument '") + arg + "'";
            parse_result.result.error_keys.push_back("cli_unknown_argument");
            return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
        } else {
            // Unexpected positional arg
            parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
            parse_result.result.message_key = "cli_unknown_argument";
            parse_result.result.summary_text = std::string("Error: unknown argument '") + arg + "'";
            parse_result.result.error_keys.push_back("cli_unknown_argument");
            return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
        }

        ++i;
    }

    // Default command if not set
    if (opts.command == AgentDebugCliCommand::Unknown) {
        opts.command = AgentDebugCliCommand::Export;
    }

    // Validate options
    auto ov = opts.validateBasic();
    if (ov.is_error()) {
        parse_result.result.exit_code = AgentDebugCliExitCode::InvalidArguments;
        parse_result.result.message_key = ov.errors().empty() ? "cli_validation_failed" : ov.errors()[0].message_key;
        parse_result.result.summary_text = "Error: " + parse_result.result.message_key;
        parse_result.result.error_keys.push_back(parse_result.result.message_key);
        return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
    }

    // Success - should execute
    parse_result.should_execute = true;
    parse_result.result.exit_code = AgentDebugCliExitCode::Success;
    parse_result.result.message_key = "cli_parse_ok";
    parse_result.result.summary_text = "Parse OK";
    return pathfinder::foundation::Result<AgentDebugCliParseResult>::ok(std::move(parse_result));
}

// --- AgentDebugFixtureFactory::build ---

pathfinder::foundation::Result<AgentDebugFixtureBundle> AgentDebugFixtureFactory::build(
    AgentDebugCliFixture fixture) const {

    if (fixture == AgentDebugCliFixture::Unknown) {
        return pathfinder::foundation::Result<AgentDebugFixtureBundle>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "cli_fixture_unknown"));
    }

    AgentDebugFixtureBundle bundle;

    if (fixture == AgentDebugCliFixture::UnknownFruit) {
        // Successful decision + replay locked style report
        bundle.report.report_id = makeAgentDebugReportId("cli_unknown_fruit", 1);
        bundle.report.mode = AgentDebugReportMode::Debug;
        bundle.report.total_items = 1;
        bundle.report.replay_locked_count = 1;

        AgentDebugReportItem item;
        item.record_id = "record_unknown_fruit_001";
        item.agent_id = "agent_spider_01";
        item.actor_id = "actor_spider_01";
        item.tick = 42;
        item.runtime_status = "PipelineSucceeded";
        item.decision_status = "Decided";
        item.command_id = "cmd_eat_unknown_fruit";
        item.replay_lock_status = "Locked";
        item.training_sample_status = "ReplayLocked";
        item.severity = AgentDebugReportSeverity::Info;
        item.summary_keys.push_back("pipeline_succeeded");
        item.summary_keys.push_back("decision_made");
        item.summary_keys.push_back("replay_locked");
        bundle.report.items.push_back(item);

        bundle.diagnostics.status = AgentDiagnosticsStatus::Clean;
        bundle.diagnostics.item_count = 1;
        bundle.diagnostics.warning_count = 0;
        bundle.diagnostics.error_count = 0;
    } else if (fixture == AgentDebugCliFixture::NoDecision) {
        // No-decision report
        bundle.report.report_id = makeAgentDebugReportId("cli_no_decision", 1);
        bundle.report.mode = AgentDebugReportMode::Debug;
        bundle.report.total_items = 1;
        bundle.report.no_decision_count = 1;

        AgentDebugReportItem item;
        item.record_id = "record_no_decision_001";
        item.agent_id = "agent_spider_02";
        item.actor_id = "actor_spider_02";
        item.tick = 10;
        item.runtime_status = "SubmitSkipped";
        item.decision_status = "NoDecision";
        item.severity = AgentDebugReportSeverity::Warning;
        item.summary_keys.push_back("no_decision");
        item.reason_keys.push_back("no_action_candidates");
        item.phase_keys.push_back("action_space_empty");
        bundle.report.items.push_back(item);

        bundle.diagnostics.status = AgentDiagnosticsStatus::HasWarnings;
        bundle.diagnostics.item_count = 1;
        bundle.diagnostics.warning_count = 1;

        AgentDiagnosticsIssue issue;
        issue.severity = AgentDebugReportSeverity::Warning;
        issue.issue_key = "no_decision_no_explanation";
        issue.record_id = "record_no_decision_001";
        issue.agent_id = "agent_spider_02";
        bundle.diagnostics.issues.push_back(issue);
    } else if (fixture == AgentDebugCliFixture::PublicSafe) {
        // Public mode, no trace keys
        bundle.report.report_id = makeAgentDebugReportId("cli_public_safe", 1);
        bundle.report.mode = AgentDebugReportMode::Public;
        bundle.report.total_items = 1;
        bundle.report.replay_locked_count = 1;

        AgentDebugReportItem item;
        item.record_id = "record_public_safe_001";
        item.agent_id = "agent_wolf_01";
        item.actor_id = "actor_wolf_01";
        item.tick = 5;
        item.runtime_status = "PipelineSucceeded";
        item.decision_status = "Decided";
        item.command_id = "cmd_flee";
        item.replay_lock_status = "Locked";
        item.severity = AgentDebugReportSeverity::Info;
        item.summary_keys.push_back("pipeline_succeeded");
        // No reason_keys, phase_keys, warning_keys in public mode
        bundle.report.items.push_back(item);

        bundle.diagnostics.status = AgentDiagnosticsStatus::Clean;
        bundle.diagnostics.item_count = 1;
        bundle.diagnostics.warning_count = 0;
        bundle.diagnostics.error_count = 0;
    }

    auto bv = bundle.validateBasic();
    if (bv.is_error()) {
        return pathfinder::foundation::Result<AgentDebugFixtureBundle>::fail(bv.errors());
    }

    return pathfinder::foundation::Result<AgentDebugFixtureBundle>::ok(std::move(bundle));
}

// --- AgentDebugCliRunner::run ---

pathfinder::foundation::Result<AgentDebugCliResult> AgentDebugCliRunner::run(
    const AgentDebugCliOptions& options) const {

    // Validate options
    auto ov = options.validateBasic();
    if (ov.is_error()) {
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::InvalidArguments;
        result.message_key = ov.errors().empty() ? "cli_options_invalid" : ov.errors()[0].message_key;
        result.summary_text = "Error: " + result.message_key;
        result.error_keys.push_back(result.message_key);
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(result));
    }

    // Help command
    if (options.help || options.command == AgentDebugCliCommand::Help) {
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::Success;
        result.message_key = "cli_help";
        result.summary_text = "Help: use --help for usage information";
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(result));
    }

    // Export command
    if (options.command != AgentDebugCliCommand::Export) {
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::InvalidArguments;
        result.message_key = "cli_unsupported_command";
        result.summary_text = "Error: unsupported command";
        result.error_keys.push_back("cli_unsupported_command");
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(result));
    }

    // Build fixture
    AgentDebugFixtureFactory fixture_factory;
    auto fb = fixture_factory.build(options.fixture);
    if (fb.is_error()) {
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::BuildFailed;
        result.message_key = "cli_fixture_build_failed";
        result.summary_text = "Error: fixture build failed";
        result.error_keys.push_back("cli_fixture_build_failed");
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(result));
    }

    // Build export draft
    AgentExportDraftBuildRequest draft_request;
    draft_request.report = fb.value().report;
    draft_request.diagnostics = fb.value().diagnostics;
    draft_request.format = toExportFormat(options.format);
    draft_request.max_items_per_chunk = options.max_items_per_chunk;

    AgentExportDraftBuilder draft_builder;
    auto draft_result = draft_builder.build(draft_request);
    if (draft_result.is_error()) {
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::BuildFailed;
        result.message_key = "cli_draft_build_failed";
        result.summary_text = "Error: export draft build failed";
        result.error_keys.push_back("cli_draft_build_failed");
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(result));
    }

    // Build write plan
    AgentExportPathPolicy path_policy;
    path_policy.root_dir = options.output_dir;
    path_policy.allow_overwrite = options.overwrite;

    AgentExportWritePlanRequest plan_request;
    plan_request.draft = draft_result.value();
    plan_request.path_policy = path_policy;
    plan_request.base_name = options.base_name;

    AgentExportWritePlanner planner;
    auto plan_result = planner.buildPlan(plan_request);
    if (plan_result.is_error()) {
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::BuildFailed;
        result.message_key = "cli_plan_build_failed";
        result.summary_text = "Error: write plan build failed";
        result.error_keys.push_back("cli_plan_build_failed");
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(result));
    }

    AgentDebugCliResult cli_result;
    cli_result.planned_file_count = plan_result.value().files.size();
    cli_result.output_dir = options.output_dir;

    // Dry run - don't write files
    if (options.dry_run) {
        cli_result.exit_code = AgentDebugCliExitCode::Success;
        cli_result.message_key = "cli_dry_run_success";
        std::ostringstream oss;
        oss << "command=Export"
            << " fixture=" << toString(options.fixture)
            << " format=" << toString(options.format)
            << " output_dir=" << options.output_dir
            << " dry_run=true"
            << " planned_file_count=" << cli_result.planned_file_count;
        cli_result.summary_text = oss.str();
        for (const auto& f : plan_result.value().files) {
            cli_result.output_files.push_back(f.relative_path);
        }
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(cli_result));
    }

    // Write files
    AgentLocalExportService export_service;
    auto write_result = export_service.write(plan_result.value());
    if (write_result.is_error()) {
        cli_result.exit_code = AgentDebugCliExitCode::WriteFailed;
        cli_result.message_key = "cli_write_failed";
        cli_result.summary_text = "Error: file write failed";
        cli_result.error_keys.push_back("cli_write_failed");
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(cli_result));
    }

    if (write_result.value().status == AgentExportWriteStatus::Failed) {
        cli_result.exit_code = AgentDebugCliExitCode::WriteFailed;
        cli_result.message_key = "cli_write_status_failed";
        cli_result.summary_text = "Error: write status failed";
        cli_result.error_keys.push_back("cli_write_status_failed");
        for (const auto& ek : write_result.value().error_keys) {
            cli_result.error_keys.push_back(ek);
        }
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(cli_result));
    }

    cli_result.written_file_count = write_result.value().written_file_count;
    for (const auto& f : write_result.value().files) {
        if (f.status == AgentExportWriteStatus::Written) {
            cli_result.output_files.push_back(f.relative_path);
        }
    }

    // Verify
    AgentExportVerifyRequest verify_request;
    verify_request.plan = plan_result.value();
    verify_request.write_result = write_result.value();
    verify_request.scan_content_for_forbidden_terms = options.scan_content;

    AgentExportVerifier verifier;
    auto verify_result = verifier.verify(verify_request);
    if (verify_result.is_error()) {
        cli_result.exit_code = AgentDebugCliExitCode::VerificationFailed;
        cli_result.message_key = "cli_verify_failed";
        cli_result.summary_text = "Error: verification failed";
        cli_result.error_keys.push_back("cli_verify_failed");
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(cli_result));
    }

    if (verify_result.value().status == AgentExportVerificationStatus::Failed) {
        cli_result.exit_code = AgentDebugCliExitCode::VerificationFailed;
        cli_result.message_key = "cli_verification_status_failed";
        cli_result.summary_text = "Error: verification status failed";
        cli_result.error_keys.push_back("cli_verification_status_failed");
        for (const auto& ek : verify_result.value().error_keys) {
            cli_result.error_keys.push_back(ek);
        }
        return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(cli_result));
    }

    // Success
    cli_result.exit_code = AgentDebugCliExitCode::Success;
    cli_result.message_key = "cli_export_success";
    std::ostringstream oss;
    oss << "command=Export"
        << " fixture=" << toString(options.fixture)
        << " format=" << toString(options.format)
        << " output_dir=" << options.output_dir
        << " file_count=" << cli_result.written_file_count
        << " verification=Passed";
    cli_result.summary_text = oss.str();

    return pathfinder::foundation::Result<AgentDebugCliResult>::ok(std::move(cli_result));
}

} // namespace pathfinder::agent
