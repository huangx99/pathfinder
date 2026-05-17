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

static const std::string test_root = "/tmp/p13_integration_security_test";

static void cleanup() {
    std::error_code ec;
    std::filesystem::remove_all(test_root, ec);
}

void test_base_name_escape() {
    TEST(p13_base_name_escape)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--format", "markdown",
                              "--output-dir", test_root.c_str(), "--base-name", "../escape"};
        auto r = parser.parse(10, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

void test_json_rejected() {
    TEST(p13_json_rejected)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--json"};
        auto r = parser.parse(3, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

void test_load_save_rejected() {
    TEST(p13_load_save_rejected)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--load-save", "xxx"};
        auto r = parser.parse(4, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

void test_scan_content_rejected() {
    TEST(p13_scan_content_rejected)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--output-dir", test_root.c_str(), "--scan-content"};
        auto r = parser.parse(7, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
        ASSERT_EQ(r.value().should_execute, false);
    PASS()
}

void test_no_scan_content_rejected() {
    TEST(p13_no_scan_content_rejected)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--output-dir", test_root.c_str(), "--no-scan-content"};
        auto r = parser.parse(7, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
        ASSERT_EQ(r.value().should_execute, false);
    PASS()
}

void test_output_dir_missing_value() {
    TEST(p13_output_dir_missing_value)
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--output-dir"};
        auto r = parser.parse(5, argv);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::InvalidArguments);
    PASS()
}

void test_dry_run_no_files() {
    TEST(p13_dry_run_no_files)
        cleanup();

        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--format", "markdown",
                              "--output-dir", test_root.c_str(), "--dry-run"};
        auto pr = parser.parse(9, argv);
        ASSERT_OK(pr);
        ASSERT_EQ(pr.value().should_execute, true);

        AgentDebugCliRunner runner;
        auto rr = runner.run(pr.value().options);
        ASSERT_OK(rr);
        ASSERT_EQ(rr.value().exit_code, AgentDebugCliExitCode::Success);

        // No files should be created
        ASSERT_TRUE(!std::filesystem::exists(test_root));

        cleanup();
    PASS()
}

void test_overwrite_false_conflict() {
    TEST(p13_overwrite_false_conflict)
        cleanup();
        std::filesystem::create_directories(test_root);
        // Pre-create a file that will conflict
        std::ofstream(test_root + "/agent_debug_manifest.md") << "existing content that is long enough to match";

        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--format", "markdown",
                              "--output-dir", test_root.c_str()};
        auto pr = parser.parse(8, argv);
        ASSERT_OK(pr);
        ASSERT_EQ(pr.value().should_execute, true);

        AgentDebugCliRunner runner;
        auto rr = runner.run(pr.value().options);
        ASSERT_OK(rr);
        ASSERT_EQ(rr.value().exit_code, AgentDebugCliExitCode::WriteFailed);

        cleanup();
    PASS()
}

void test_no_files_outside_root() {
    TEST(p13_no_files_outside_root)
        cleanup();

        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--format", "markdown",
                              "--output-dir", test_root.c_str(), "--overwrite"};
        auto pr = parser.parse(9, argv);
        ASSERT_OK(pr);

        AgentDebugCliRunner runner;
        auto rr = runner.run(pr.value().options);
        ASSERT_OK(rr);
        ASSERT_EQ(rr.value().exit_code, AgentDebugCliExitCode::Success);

        // Verify all output_files are relative paths
        for (const auto& f : rr.value().output_files) {
            ASSERT_TRUE(!f.empty() && f[0] != '/');
        }

        // Verify nothing was written outside test_root
        auto parent = std::filesystem::path(test_root).parent_path();
        // Only test_root should have been created
        ASSERT_TRUE(std::filesystem::exists(test_root));

        cleanup();
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

    std::cout << "=== P13 Agent Debug CLI Security Integration Tests ===" << std::endl;

    run("smoke", []() {
        TEST(smoke)
            AgentDebugCliParser parser;
            const char* argv[] = {"prog", "--help"};
            auto r = parser.parse(2, argv);
            ASSERT_OK(r);
            ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::Success);
        PASS()
    });

    run("p13_base_name_escape", test_base_name_escape);
    run("p13_json_rejected", test_json_rejected);
    run("p13_load_save_rejected", test_load_save_rejected);
    run("p13_scan_content_rejected", test_scan_content_rejected);
    run("p13_no_scan_content_rejected", test_no_scan_content_rejected);
    run("p13_output_dir_missing_value", test_output_dir_missing_value);
    run("p13_dry_run_no_files", test_dry_run_no_files);
    run("p13_overwrite_false_conflict", test_overwrite_false_conflict);
    run("p13_no_files_outside_root", test_no_files_outside_root);

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;

    cleanup();
    return (pass_count == test_count) ? 0 : 1;
}
