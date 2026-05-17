#include "pathfinder/agent/debug_cli.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) \
    do { \
        ++test_count; \
        std::string test_name = #name; \
        try {

#define PASS() \
            ++pass_count; \
            std::cout << "  PASS: " << test_name << std::endl; \
        } catch (const std::exception& e) { \
            std::cout << "  FAIL: " << test_name << " - " << e.what() << std::endl; \
        } \
    } while(0);

#define ASSERT_OK(result) \
    if ((result).is_error()) { \
        std::string msg = "expected ok but got error"; \
        if (!(result).errors().empty()) msg = (result).errors()[0].message_key; \
        throw std::runtime_error(msg); \
    }

#define ASSERT_ERROR(result) \
    if ((result).is_ok()) { \
        throw std::runtime_error("expected error but got ok"); \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error("assertion failed: " + std::to_string(__LINE__)); \
    }

#define ASSERT_TRUE(x) \
    if (!(x)) { \
        throw std::runtime_error("assertion failed: " + std::to_string(__LINE__)); \
    }

// --- Enum roundtrip tests ---

void test_command_roundtrip() {
    TEST(command_roundtrip)
        ASSERT_EQ(toString(AgentDebugCliCommand::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentDebugCliCommand::Help), "Help");
        ASSERT_EQ(toString(AgentDebugCliCommand::Export), "Export");

        auto r1 = agentDebugCliCommandFromString("Unknown");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentDebugCliCommand::Unknown);

        auto r2 = agentDebugCliCommandFromString("Help");
        ASSERT_OK(r2);
        ASSERT_EQ(r2.value(), AgentDebugCliCommand::Help);

        auto r3 = agentDebugCliCommandFromString("Export");
        ASSERT_OK(r3);
        ASSERT_EQ(r3.value(), AgentDebugCliCommand::Export);

        auto r4 = agentDebugCliCommandFromString("Bogus");
        ASSERT_ERROR(r4);
    PASS()
}

void test_fixture_roundtrip() {
    TEST(fixture_roundtrip)
        ASSERT_EQ(toString(AgentDebugCliFixture::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentDebugCliFixture::UnknownFruit), "UnknownFruit");
        ASSERT_EQ(toString(AgentDebugCliFixture::NoDecision), "NoDecision");
        ASSERT_EQ(toString(AgentDebugCliFixture::PublicSafe), "PublicSafe");

        auto r1 = agentDebugCliFixtureFromString("unknown_fruit");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentDebugCliFixture::UnknownFruit);

        auto r2 = agentDebugCliFixtureFromString("no_decision");
        ASSERT_OK(r2);
        ASSERT_EQ(r2.value(), AgentDebugCliFixture::NoDecision);

        auto r3 = agentDebugCliFixtureFromString("public_safe");
        ASSERT_OK(r3);
        ASSERT_EQ(r3.value(), AgentDebugCliFixture::PublicSafe);

        auto r4 = agentDebugCliFixtureFromString("Bogus");
        ASSERT_ERROR(r4);
    PASS()
}

void test_format_roundtrip() {
    TEST(format_roundtrip)
        ASSERT_EQ(toString(AgentDebugCliFormat::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentDebugCliFormat::Text), "Text");
        ASSERT_EQ(toString(AgentDebugCliFormat::Markdown), "Markdown");
        ASSERT_EQ(toString(AgentDebugCliFormat::ProtocolText), "ProtocolText");

        auto r1 = agentDebugCliFormatFromString("text");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentDebugCliFormat::Text);

        auto r2 = agentDebugCliFormatFromString("markdown");
        ASSERT_OK(r2);
        ASSERT_EQ(r2.value(), AgentDebugCliFormat::Markdown);

        auto r3 = agentDebugCliFormatFromString("protocol_text");
        ASSERT_OK(r3);
        ASSERT_EQ(r3.value(), AgentDebugCliFormat::ProtocolText);

        auto r4 = agentDebugCliFormatFromString("json");
        ASSERT_ERROR(r4);
    PASS()
}

void test_exit_code_int() {
    TEST(exit_code_int)
        ASSERT_EQ(toInt(AgentDebugCliExitCode::Success), 0);
        ASSERT_EQ(toInt(AgentDebugCliExitCode::InvalidArguments), 2);
        ASSERT_EQ(toInt(AgentDebugCliExitCode::BuildFailed), 3);
        ASSERT_EQ(toInt(AgentDebugCliExitCode::WriteFailed), 4);
        ASSERT_EQ(toInt(AgentDebugCliExitCode::VerificationFailed), 5);
        ASSERT_EQ(toInt(AgentDebugCliExitCode::InternalError), 10);
    PASS()
}

void test_format_conversion() {
    TEST(format_conversion)
        ASSERT_EQ(toExportFormat(AgentDebugCliFormat::Text), AgentExportFormat::PlainText);
        ASSERT_EQ(toExportFormat(AgentDebugCliFormat::Markdown), AgentExportFormat::MarkdownLike);
        ASSERT_EQ(toExportFormat(AgentDebugCliFormat::ProtocolText), AgentExportFormat::ProtocolText);
        ASSERT_EQ(toExportFormat(AgentDebugCliFormat::Unknown), AgentExportFormat::Unknown);
    PASS()
}

// --- Options validation tests ---

void test_options_unknown_command() {
    TEST(options_unknown_command_fails)
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Unknown;
        auto r = opts.validateBasic();
        ASSERT_ERROR(r);
    PASS()
}

void test_options_export_missing_fixture() {
    TEST(options_export_missing_fixture_fails)
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::Unknown;
        opts.output_dir = "/tmp/test";
        auto r = opts.validateBasic();
        ASSERT_ERROR(r);
    PASS()
}

void test_options_export_missing_output_dir() {
    TEST(options_export_missing_output_dir_fails)
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::UnknownFruit;
        opts.output_dir = "";
        auto r = opts.validateBasic();
        ASSERT_ERROR(r);
    PASS()
}

void test_options_invalid_base_name() {
    TEST(options_invalid_base_name_fails)
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::UnknownFruit;
        opts.output_dir = "/tmp/test";
        opts.base_name = "../escape";
        auto r = opts.validateBasic();
        ASSERT_ERROR(r);
    PASS()
}

void test_options_max_items_zero() {
    TEST(options_max_items_zero_fails)
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::UnknownFruit;
        opts.output_dir = "/tmp/test";
        opts.max_items_per_chunk = 0;
        auto r = opts.validateBasic();
        ASSERT_ERROR(r);
    PASS()
}

void test_options_help_valid() {
    TEST(options_help_valid)
        AgentDebugCliOptions opts;
        opts.help = true;
        auto r = opts.validateBasic();
        ASSERT_OK(r);
    PASS()
}

// --- Result validation tests ---

void test_result_success_with_errors() {
    TEST(result_success_with_error_keys_fails)
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::Success;
        result.summary_text = "ok";
        result.error_keys.push_back("some_error");
        auto r = result.validateBasic();
        ASSERT_ERROR(r);
    PASS()
}

void test_result_failure_without_errors() {
    TEST(result_failure_without_error_keys_fails)
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::BuildFailed;
        result.summary_text = "failed";
        auto r = result.validateBasic();
        ASSERT_ERROR(r);
    PASS()
}

void test_result_absolute_path_in_output() {
    TEST(result_absolute_path_in_output_files_fails)
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::Success;
        result.summary_text = "ok";
        result.output_files.push_back("/absolute/path");
        auto r = result.validateBasic();
        ASSERT_ERROR(r);
    PASS()
}

void test_result_empty_summary() {
    TEST(result_empty_summary_text_fails)
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::Success;
        result.summary_text = "";
        auto r = result.validateBasic();
        ASSERT_ERROR(r);
    PASS()
}

void test_result_valid_success() {
    TEST(result_valid_success_passes)
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::Success;
        result.summary_text = "ok";
        auto r = result.validateBasic();
        ASSERT_OK(r);
    PASS()
}

void test_result_valid_failure() {
    TEST(result_valid_failure_passes)
        AgentDebugCliResult result;
        result.exit_code = AgentDebugCliExitCode::BuildFailed;
        result.summary_text = "failed";
        result.error_keys.push_back("some_error");
        auto r = result.validateBasic();
        ASSERT_OK(r);
    PASS()
}

// --- Parser tests ---

void test_parser_help() {
    TEST(parser_help)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "--help"};
        auto r = parser.parse(2, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().should_execute, false);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::Success);
        ASSERT_TRUE(r.value().options.help);
    PASS()
}

void test_parser_export_unknown_fruit() {
    TEST(parser_export_unknown_fruit)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--format", "markdown", "--output-dir", "/tmp/test"};
        auto r = parser.parse(8, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().should_execute, true);
        ASSERT_EQ(r.value().options.command, AgentDebugCliCommand::Export);
        ASSERT_EQ(r.value().options.fixture, AgentDebugCliFixture::UnknownFruit);
        ASSERT_EQ(r.value().options.format, AgentDebugCliFormat::Markdown);
        ASSERT_EQ(r.value().options.output_dir, "/tmp/test");
    PASS()
}

void test_parser_dry_run() {
    TEST(parser_dry_run)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "no_decision", "--format", "text", "--output-dir", "/tmp/test", "--dry-run"};
        auto r = parser.parse(9, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().should_execute, true);
        ASSERT_TRUE(r.value().options.dry_run);
    PASS()
}

void test_parser_overwrite() {
    TEST(parser_overwrite)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--output-dir", "/tmp/test", "--overwrite"};
        auto r = parser.parse(7, argv);
        ASSERT_OK(r);
        ASSERT_TRUE(r.value().options.overwrite);
    PASS()
}

void test_parser_unknown_arg() {
    TEST(parser_unknown_arg_fails)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--bogus"};
        auto r = parser.parse(3, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

void test_parser_json_rejected() {
    TEST(parser_json_fails)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--json"};
        auto r = parser.parse(3, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

void test_parser_load_save_rejected() {
    TEST(parser_load_save_fails)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--load-save", "xxx"};
        auto r = parser.parse(4, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

void test_parser_scan_content_rejected() {
    TEST(parser_scan_content_fails)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--output-dir", "/tmp/test", "--scan-content"};
        auto r = parser.parse(7, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
        ASSERT_EQ(r.value().should_execute, false);
    PASS()
}

void test_parser_no_scan_content_rejected() {
    TEST(parser_no_scan_content_fails)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--output-dir", "/tmp/test", "--no-scan-content"};
        auto r = parser.parse(7, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
        ASSERT_EQ(r.value().should_execute, false);
    PASS()
}

void test_parser_fixture_missing_value() {
    TEST(parser_fixture_missing_value_fails)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture"};
        auto r = parser.parse(3, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

void test_parser_max_items_non_number() {
    TEST(parser_max_items_non_number_fails)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--output-dir", "/tmp/test", "--max-items-per-chunk", "abc"};
        auto r = parser.parse(8, argv);
        ASSERT_TRUE(r.is_ok());
        ASSERT_TRUE(r.value().result.exit_code == AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

void test_parser_base_name_escape() {
    TEST(parser_base_name_escape_fails)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--output-dir", "/tmp/test", "--base-name", "../escape"};
        auto r = parser.parse(8, argv);
        ASSERT_TRUE(r.is_ok());
        ASSERT_TRUE(r.value().result.exit_code == AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

// --- FixtureFactory tests ---

void test_fixture_unknown_fruit() {
    TEST(fixture_unknown_fruit_valid)
        AgentDebugFixtureFactory factory;
        auto r = factory.build(AgentDebugCliFixture::UnknownFruit);
        ASSERT_OK(r);
        ASSERT_OK(r.value().validateBasic());
        ASSERT_EQ(r.value().report.mode, AgentDebugReportMode::Debug);
        ASSERT_EQ(r.value().report.total_items, 1u);
        ASSERT_EQ(r.value().report.replay_locked_count, 1u);
    PASS()
}

void test_fixture_no_decision() {
    TEST(fixture_no_decision_valid)
        AgentDebugFixtureFactory factory;
        auto r = factory.build(AgentDebugCliFixture::NoDecision);
        ASSERT_OK(r);
        ASSERT_OK(r.value().validateBasic());
        ASSERT_EQ(r.value().report.no_decision_count, 1u);
        ASSERT_EQ(r.value().diagnostics.status, AgentDiagnosticsStatus::HasWarnings);
    PASS()
}

void test_fixture_public_safe() {
    TEST(fixture_public_safe_valid)
        AgentDebugFixtureFactory factory;
        auto r = factory.build(AgentDebugCliFixture::PublicSafe);
        ASSERT_OK(r);
        ASSERT_OK(r.value().validateBasic());
        ASSERT_EQ(r.value().report.mode, AgentDebugReportMode::Public);
        // Public mode items should not have reason_keys/phase_keys/warning_keys
        for (const auto& item : r.value().report.items) {
            ASSERT_TRUE(item.reason_keys.empty());
            ASSERT_TRUE(item.phase_keys.empty());
            ASSERT_TRUE(item.warning_keys.empty());
        }
    PASS()
}

void test_fixture_unknown_fails() {
    TEST(fixture_unknown_fails)
        AgentDebugFixtureFactory factory;
        auto r = factory.build(AgentDebugCliFixture::Unknown);
        ASSERT_ERROR(r);
    PASS()
}

void test_fixture_no_hidden_truth() {
    TEST(fixture_no_hidden_truth)
        AgentDebugFixtureFactory factory;
        auto fixtures = {AgentDebugCliFixture::UnknownFruit, AgentDebugCliFixture::NoDecision, AgentDebugCliFixture::PublicSafe};
        for (auto f : fixtures) {
            auto r = factory.build(f);
            ASSERT_OK(r);
            // Check summary_keys don't contain hidden truth terms
            for (const auto& item : r.value().report.items) {
                for (const auto& key : item.summary_keys) {
                    ASSERT_TRUE(key.find("GameState") == std::string::npos);
                    ASSERT_TRUE(key.find("ObjectDefinition") == std::string::npos);
                    ASSERT_TRUE(key.find("edible_profile") == std::string::npos);
                    ASSERT_TRUE(key.find("reward_value") == std::string::npos);
                }
            }
        }
    PASS()
}

// --- Runner tests ---

void test_runner_help() {
    TEST(runner_help)
        AgentDebugCliRunner runner;
        AgentDebugCliOptions opts;
        opts.help = true;
        opts.command = AgentDebugCliCommand::Help;
        auto r = runner.run(opts);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().exit_code, AgentDebugCliExitCode::Success);
        ASSERT_TRUE(!r.value().summary_text.empty());
    PASS()
}

static const std::string test_root = "/tmp/p13_cli_unit_test";

static void cleanup_test_dir() {
    std::error_code ec;
    std::filesystem::remove_all(test_root, ec);
}

void test_runner_dry_run() {
    TEST(runner_dry_run)
        cleanup_test_dir();

        AgentDebugCliRunner runner;
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::UnknownFruit;
        opts.format = AgentDebugCliFormat::Markdown;
        opts.output_dir = test_root;
        opts.dry_run = true;

        auto r = runner.run(opts);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().exit_code, AgentDebugCliExitCode::Success);
        ASSERT_TRUE(r.value().summary_text.find("dry_run=true") != std::string::npos);
        ASSERT_TRUE(r.value().planned_file_count > 0);
        // No files should be written
        ASSERT_EQ(r.value().written_file_count, 0u);
        // Directory should not be created
        ASSERT_TRUE(!std::filesystem::exists(test_root));

        cleanup_test_dir();
    PASS()
}

void test_runner_unknown_fruit_markdown() {
    TEST(runner_unknown_fruit_markdown)
        cleanup_test_dir();

        AgentDebugCliRunner runner;
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::UnknownFruit;
        opts.format = AgentDebugCliFormat::Markdown;
        opts.output_dir = test_root;
        opts.overwrite = true;

        auto r = runner.run(opts);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().exit_code, AgentDebugCliExitCode::Success);
        ASSERT_TRUE(r.value().written_file_count > 0);
        ASSERT_TRUE(r.value().summary_text.find("verification=Passed") != std::string::npos);

        cleanup_test_dir();
    PASS()
}

void test_runner_no_decision_text() {
    TEST(runner_no_decision_text)
        cleanup_test_dir();

        AgentDebugCliRunner runner;
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::NoDecision;
        opts.format = AgentDebugCliFormat::Text;
        opts.output_dir = test_root;
        opts.overwrite = true;

        auto r = runner.run(opts);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().exit_code, AgentDebugCliExitCode::Success);

        cleanup_test_dir();
    PASS()
}

void test_runner_public_safe_protocol_text() {
    TEST(runner_public_safe_protocol_text)
        cleanup_test_dir();

        AgentDebugCliRunner runner;
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::PublicSafe;
        opts.format = AgentDebugCliFormat::ProtocolText;
        opts.output_dir = test_root;
        opts.overwrite = true;

        auto r = runner.run(opts);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().exit_code, AgentDebugCliExitCode::Success);

        cleanup_test_dir();
    PASS()
}

void test_runner_no_overwrite_conflict() {
    TEST(runner_no_overwrite_conflict)
        cleanup_test_dir();
        std::filesystem::create_directories(test_root);
        // Create a file that would conflict
        std::ofstream(test_root + "/agent_debug_manifest.md") << "existing content that is long enough";

        AgentDebugCliRunner runner;
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::UnknownFruit;
        opts.format = AgentDebugCliFormat::Markdown;
        opts.output_dir = test_root;
        opts.overwrite = false;

        auto r = runner.run(opts);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().exit_code, AgentDebugCliExitCode::WriteFailed);

        cleanup_test_dir();
    PASS()
}

void test_runner_invalid_output_dir() {
    TEST(runner_invalid_output_dir)
        AgentDebugCliRunner runner;
        AgentDebugCliOptions opts;
        opts.command = AgentDebugCliCommand::Export;
        opts.fixture = AgentDebugCliFixture::UnknownFruit;
        opts.output_dir = "/nonexistent_root_dir_that_should_not_exist_12345/escape/..";

        auto r = runner.run(opts);
        ASSERT_OK(r);
        // Should fail - either BuildFailed (plan validation) or WriteFailed
        ASSERT_TRUE(r.value().exit_code == AgentDebugCliExitCode::BuildFailed ||
                    r.value().exit_code == AgentDebugCliExitCode::WriteFailed);
    PASS()
}

// --- Main ---

int main(int argc, char* argv[]) {
    std::string filter = (argc > 1) ? argv[1] : "";

    auto run = [&](const std::string& name, void(*fn)()) {
        if (filter.empty() || filter == name) {
            fn();
        }
    };

    std::cout << "=== Agent Debug CLI Tests (P13) ===" << std::endl;

    run("smoke", []() {
        TEST(smoke)
            AgentDebugCliCommand c = AgentDebugCliCommand::Export;
            ASSERT_EQ(toString(c), "Export");
        PASS()
    });

    // Enum roundtrip
    run("command_roundtrip", test_command_roundtrip);
    run("fixture_roundtrip", test_fixture_roundtrip);
    run("format_roundtrip", test_format_roundtrip);
    run("exit_code_int", test_exit_code_int);
    run("format_conversion", test_format_conversion);

    // Options validation
    run("options_unknown_command_fails", test_options_unknown_command);
    run("options_export_missing_fixture_fails", test_options_export_missing_fixture);
    run("options_export_missing_output_dir_fails", test_options_export_missing_output_dir);
    run("options_invalid_base_name_fails", test_options_invalid_base_name);
    run("options_max_items_zero_fails", test_options_max_items_zero);
    run("options_help_valid", test_options_help_valid);

    // Result validation
    run("result_success_with_error_keys_fails", test_result_success_with_errors);
    run("result_failure_without_error_keys_fails", test_result_failure_without_errors);
    run("result_absolute_path_in_output_files_fails", test_result_absolute_path_in_output);
    run("result_empty_summary_text_fails", test_result_empty_summary);
    run("result_valid_success_passes", test_result_valid_success);
    run("result_valid_failure_passes", test_result_valid_failure);

    // Parser
    run("parser_help", test_parser_help);
    run("parser_export_unknown_fruit_markdown", test_parser_export_unknown_fruit);
    run("parser_dry_run", test_parser_dry_run);
    run("parser_overwrite", test_parser_overwrite);
    run("parser_unknown_arg_fails", test_parser_unknown_arg);
    run("parser_json_fails", test_parser_json_rejected);
    run("parser_load_save_fails", test_parser_load_save_rejected);
    run("parser_scan_content_fails", test_parser_scan_content_rejected);
    run("parser_no_scan_content_fails", test_parser_no_scan_content_rejected);
    run("parser_fixture_missing_value_fails", test_parser_fixture_missing_value);
    run("parser_max_items_non_number_fails", test_parser_max_items_non_number);
    run("parser_base_name_escape_fails", test_parser_base_name_escape);

    // Fixture
    run("fixture_unknown_fruit_valid", test_fixture_unknown_fruit);
    run("fixture_no_decision_valid", test_fixture_no_decision);
    run("fixture_public_safe_valid", test_fixture_public_safe);
    run("fixture_unknown_fails", test_fixture_unknown_fails);
    run("fixture_no_hidden_truth", test_fixture_no_hidden_truth);

    // Runner
    run("runner_help", test_runner_help);
    run("runner_dry_run", test_runner_dry_run);
    run("runner_unknown_fruit_markdown", test_runner_unknown_fruit_markdown);
    run("runner_no_decision_text", test_runner_no_decision_text);
    run("runner_public_safe_protocol_text", test_runner_public_safe_protocol_text);
    run("runner_no_overwrite_conflict", test_runner_no_overwrite_conflict);
    run("runner_invalid_output_dir", test_runner_invalid_output_dir);

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;

    cleanup_test_dir();
    return (pass_count == test_count) ? 0 : 1;
}
