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

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error("assertion failed: " + std::to_string(__LINE__)); \
    }

#define ASSERT_TRUE(x) \
    if (!(x)) { \
        throw std::runtime_error("assertion failed: " + std::to_string(__LINE__)); \
    }

static const std::string test_root = "/tmp/p13_integration_flow_test";

static void cleanup() {
    std::error_code ec;
    std::filesystem::remove_all(test_root, ec);
}

static bool fileContains(const std::string& path, const std::string& term) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    return content.find(term) != std::string::npos;
}

void test_unknown_fruit_export_flow() {
    TEST(p13_agent_debug_cli_flow)
        cleanup();

        // Parse
        AgentDebugCliParser parser;
        const char* argv[] = {"prog", "export", "--fixture", "unknown_fruit", "--format", "markdown",
                              "--output-dir", test_root.c_str(), "--base-name", "unknown_fruit", "--overwrite"};
        auto pr = parser.parse(11, argv);
        ASSERT_OK(pr);
        ASSERT_EQ(pr.value().should_execute, true);
        ASSERT_EQ(pr.value().result.exit_code, AgentDebugCliExitCode::Success);

        // Execute
        AgentDebugCliRunner runner;
        auto rr = runner.run(pr.value().options);
        ASSERT_OK(rr);
        ASSERT_EQ(rr.value().exit_code, AgentDebugCliExitCode::Success);

        // Verify output_files contains manifest and chunk
        ASSERT_TRUE(rr.value().output_files.size() >= 2);
        bool has_manifest = false;
        bool has_chunk = false;
        for (const auto& f : rr.value().output_files) {
            if (f.find("manifest") != std::string::npos) has_manifest = true;
            if (f.find("chunk") != std::string::npos) has_chunk = true;
        }
        ASSERT_TRUE(has_manifest);
        ASSERT_TRUE(has_chunk);

        // Verify files exist
        ASSERT_TRUE(std::filesystem::exists(test_root + "/unknown_fruit_manifest.md"));
        ASSERT_TRUE(std::filesystem::exists(test_root + "/unknown_fruit_chunk_0.md"));

        // Verify summary contains verification=Passed
        ASSERT_TRUE(rr.value().summary_text.find("verification=Passed") != std::string::npos);

        // Verify no hidden truth in exported files
        ASSERT_TRUE(!fileContains(test_root + "/unknown_fruit_manifest.md", "GameState"));
        ASSERT_TRUE(!fileContains(test_root + "/unknown_fruit_manifest.md", "ObjectDefinition"));
        ASSERT_TRUE(!fileContains(test_root + "/unknown_fruit_chunk_0.md", "GameState"));
        ASSERT_TRUE(!fileContains(test_root + "/unknown_fruit_chunk_0.md", "ObjectDefinition"));
        ASSERT_TRUE(!fileContains(test_root + "/unknown_fruit_chunk_0.md", "edible_profile"));
        ASSERT_TRUE(!fileContains(test_root + "/unknown_fruit_chunk_0.md", "reward_value"));

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

    std::cout << "=== P13 Agent Debug CLI Flow Integration Tests ===" << std::endl;

    run("smoke", []() {
        TEST(smoke)
            AgentDebugCliParser parser;
            const char* argv[] = {"prog", "--help"};
            auto r = parser.parse(2, argv);
            ASSERT_OK(r);
            ASSERT_EQ(r.value().result.exit_code, AgentDebugCliExitCode::Success);
        PASS()
    });

    run("p13_agent_debug_cli_flow", test_unknown_fruit_export_flow);

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;

    cleanup();
    return (pass_count == test_count) ? 0 : 1;
}
