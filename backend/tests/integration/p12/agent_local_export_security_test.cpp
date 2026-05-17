#include "pathfinder/agent/local_export.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>

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

static const std::string test_root = "/tmp/p12_integration_security_test";

static void cleanup() {
    std::error_code ec;
    std::filesystem::remove_all(test_root, ec);
}

static AgentExportDraft makeValidDraft() {
    AgentExportChunk chunk;
    chunk.chunk_id = "sec_chunk_0";
    chunk.format = AgentExportFormat::MarkdownLike;
    chunk.title_key = "sec_title";
    chunk.payload = "Safe content for security test";
    chunk.item_count = 1;

    AgentExportManifest manifest;
    manifest.manifest_id = "sec_export_001";
    manifest.format = AgentExportFormat::MarkdownLike;
    manifest.chunk_count = 1;
    manifest.total_item_count = 1;
    manifest.chunk_ids.push_back("sec_chunk_0");

    AgentExportDraft draft;
    draft.manifest = manifest;
    draft.chunks.push_back(chunk);
    return draft;
}

void test_base_name_traversal() {
    TEST(p12_path_security)
        cleanup();

        auto draft = makeValidDraft();

        // Test 1: base_name with ../ should fail at buildPlan
        {
            AgentExportPathPolicy policy;
            policy.root_dir = test_root;
            policy.allow_create_directories = true;

            AgentExportWritePlanRequest req;
            req.draft = draft;
            req.path_policy = policy;
            req.base_name = "../escape";

            AgentExportWritePlanner planner;
            auto result = planner.buildPlan(req);
            ASSERT_ERROR(result);
        }

        // Test 2: manually crafted relative_path with traversal should fail at validateBasic
        {
            AgentExportFilePlan fp;
            fp.file_id = makeAgentExportFileId("sec", "manifest", 0);
            fp.kind = AgentExportFileKind::Manifest;
            fp.extension = AgentExportFileExtension::Md;
            fp.relative_path = "../escape.txt";
            fp.content = "malicious";
            fp.byte_size = 9;

            ASSERT_ERROR(fp.validateBasic());
        }

        // Test 3: allow_overwrite=false with existing file should fail
        {
            std::filesystem::create_directories(test_root);
            std::ofstream(test_root + "/sec_manifest.md") << "pre-existing";

            AgentExportPathPolicy policy;
            policy.root_dir = test_root;
            policy.allow_create_directories = true;
            policy.allow_overwrite = false;

            AgentExportWritePlanRequest req;
            req.draft = draft;
            req.path_policy = policy;
            req.base_name = "sec";

            AgentExportWritePlanner planner;
            auto plan_result = planner.buildPlan(req);
            ASSERT_OK(plan_result);

            AgentLocalExportService service;
            auto write_result = service.write(plan_result.value());
            ASSERT_OK(write_result);
            ASSERT_EQ(write_result.value().status, AgentExportWriteStatus::Failed);
            ASSERT_TRUE(!write_result.value().error_keys.empty());

            // Verify no new files were created outside root
            // The only file should be the pre-existing one
            size_t file_count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(test_root)) {
                if (entry.is_regular_file()) ++file_count;
            }
            ASSERT_EQ(file_count, 1u);
        }

        cleanup();
    PASS()
}

int main(int argc, char* argv[]) {
    std::string filter = (argc > 1) ? argv[1] : "";

    auto run = [&](const std::string& name, void(*fn)()) {
        if (filter.empty() || filter == name) {
            fn();
        }
    };

    std::cout << "=== P12 Integration: Path Security ===" << std::endl;

    run("smoke", []() {
        TEST(smoke)
            AgentExportFileKind k = AgentExportFileKind::Manifest;
            ASSERT_EQ(toString(k), "Manifest");
        PASS()
    });

    run("p12_path_security", test_base_name_traversal);

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;

    cleanup();
    return (pass_count == test_count) ? 0 : 1;
}
