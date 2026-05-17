#include "pathfinder/agent/local_export.h"
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

// --- Helper to build a valid draft ---

static AgentExportDraft makeValidDraft(AgentExportFormat format = AgentExportFormat::MarkdownLike) {
    AgentExportChunk chunk;
    chunk.chunk_id = "chunk_0";
    chunk.format = format;
    chunk.title_key = "title_test";
    chunk.payload = "Hello World Content";
    chunk.item_count = 1;

    AgentExportManifest manifest;
    manifest.manifest_id = "test_export_001";
    manifest.format = format;
    manifest.chunk_count = 1;
    manifest.total_item_count = 1;
    manifest.chunk_ids.push_back("chunk_0");

    AgentExportDraft draft;
    draft.manifest = manifest;
    draft.chunks.push_back(chunk);
    return draft;
}

static AgentExportPathPolicy makeValidPolicy(const std::string& root) {
    AgentExportPathPolicy policy;
    policy.root_dir = root;
    policy.allow_create_directories = true;
    policy.allow_overwrite = false;
    policy.max_file_count = 64;
    policy.max_file_bytes = 1024 * 1024;
    policy.max_total_bytes = 8 * 1024 * 1024;
    return policy;
}

// --- Enum roundtrip tests ---

void test_file_kind_roundtrip() {
    TEST(file_kind_roundtrip)
        ASSERT_EQ(toString(AgentExportFileKind::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentExportFileKind::Manifest), "Manifest");
        ASSERT_EQ(toString(AgentExportFileKind::Chunk), "Chunk");
        ASSERT_EQ(toString(AgentExportFileKind::Diagnostics), "Diagnostics");

        auto r1 = agentExportFileKindFromString("Unknown");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentExportFileKind::Unknown);

        auto r2 = agentExportFileKindFromString("Manifest");
        ASSERT_OK(r2);
        ASSERT_EQ(r2.value(), AgentExportFileKind::Manifest);

        auto r3 = agentExportFileKindFromString("Chunk");
        ASSERT_OK(r3);
        ASSERT_EQ(r3.value(), AgentExportFileKind::Chunk);

        auto r4 = agentExportFileKindFromString("Diagnostics");
        ASSERT_OK(r4);
        ASSERT_EQ(r4.value(), AgentExportFileKind::Diagnostics);

        auto r5 = agentExportFileKindFromString("Invalid");
        ASSERT_ERROR(r5);
    PASS()
}

void test_file_extension_roundtrip() {
    TEST(file_extension_roundtrip)
        ASSERT_EQ(toString(AgentExportFileExtension::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentExportFileExtension::Txt), "Txt");
        ASSERT_EQ(toString(AgentExportFileExtension::Md), "Md");

        auto r1 = agentExportFileExtensionFromString("Txt");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentExportFileExtension::Txt);

        auto r2 = agentExportFileExtensionFromString("Md");
        ASSERT_OK(r2);
        ASSERT_EQ(r2.value(), AgentExportFileExtension::Md);

        auto r3 = agentExportFileExtensionFromString("Invalid");
        ASSERT_ERROR(r3);
    PASS()
}

void test_write_status_roundtrip() {
    TEST(write_status_roundtrip)
        ASSERT_EQ(toString(AgentExportWriteStatus::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentExportWriteStatus::Planned), "Planned");
        ASSERT_EQ(toString(AgentExportWriteStatus::Written), "Written");
        ASSERT_EQ(toString(AgentExportWriteStatus::Failed), "Failed");
        ASSERT_EQ(toString(AgentExportWriteStatus::Skipped), "Skipped");

        auto r1 = agentExportWriteStatusFromString("Written");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentExportWriteStatus::Written);

        auto r2 = agentExportWriteStatusFromString("Invalid");
        ASSERT_ERROR(r2);
    PASS()
}

void test_verification_status_roundtrip() {
    TEST(verification_status_roundtrip)
        ASSERT_EQ(toString(AgentExportVerificationStatus::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentExportVerificationStatus::Passed), "Passed");
        ASSERT_EQ(toString(AgentExportVerificationStatus::Failed), "Failed");
        ASSERT_EQ(toString(AgentExportVerificationStatus::Warning), "Warning");

        auto r1 = agentExportVerificationStatusFromString("Passed");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentExportVerificationStatus::Passed);

        auto r2 = agentExportVerificationStatusFromString("Invalid");
        ASSERT_ERROR(r2);
    PASS()
}

// --- ID helper tests ---

void test_file_id_helper() {
    TEST(file_id_helper)
        auto id1 = makeAgentExportFileId("exp001", "manifest", 0);
        auto id2 = makeAgentExportFileId("exp001", "manifest", 0);
        auto id3 = makeAgentExportFileId("exp001", "chunk", 0);
        ASSERT_EQ(id1, id2);
        ASSERT_TRUE(id1 != id3);
        ASSERT_TRUE(!id1.empty());
        ASSERT_TRUE(id1.value().find("agent_export_file_exp001_manifest_0") != std::string::npos);
    PASS()
}

void test_write_id_helper() {
    TEST(write_id_helper)
        auto id1 = makeAgentExportWriteId("exp001", 3);
        auto id2 = makeAgentExportWriteId("exp001", 3);
        ASSERT_EQ(id1, id2);
        ASSERT_TRUE(id1.value().find("agent_export_write_exp001_3") != std::string::npos);
    PASS()
}

void test_verify_id_helper() {
    TEST(verify_id_helper)
        auto id1 = makeAgentExportVerifyId("write_001");
        auto id2 = makeAgentExportVerifyId("write_001");
        ASSERT_EQ(id1, id2);
        ASSERT_TRUE(id1.value().find("agent_export_verify_write_001") != std::string::npos);
    PASS()
}

// --- PathPolicy validation tests ---

void test_path_policy_empty_root() {
    TEST(path_policy_empty_root_dir_fails)
        AgentExportPathPolicy policy;
        policy.root_dir = "";
        ASSERT_ERROR(policy.validateBasic());
    PASS()
}

void test_path_policy_traversal() {
    TEST(path_policy_root_dir_traversal_fails)
        AgentExportPathPolicy policy;
        policy.root_dir = "/tmp/../escape";
        ASSERT_ERROR(policy.validateBasic());
    PASS()
}

void test_path_policy_max_file_count() {
    TEST(path_policy_max_file_count_fails)
        AgentExportPathPolicy policy;
        policy.root_dir = "/tmp/test";
        policy.max_file_count = 0;
        ASSERT_ERROR(policy.validateBasic());

        policy.max_file_count = 2000;
        ASSERT_ERROR(policy.validateBasic());
    PASS()
}

void test_path_policy_max_file_bytes() {
    TEST(path_policy_max_file_bytes_zero_fails)
        AgentExportPathPolicy policy;
        policy.root_dir = "/tmp/test";
        policy.max_file_bytes = 0;
        ASSERT_ERROR(policy.validateBasic());
    PASS()
}

void test_path_policy_total_bytes() {
    TEST(path_policy_total_bytes_less_than_max_fails)
        AgentExportPathPolicy policy;
        policy.root_dir = "/tmp/test";
        policy.max_file_bytes = 1024;
        policy.max_total_bytes = 512;
        ASSERT_ERROR(policy.validateBasic());
    PASS()
}

// --- FilePlan validation tests ---

void test_file_plan_empty_id() {
    TEST(file_plan_empty_file_id_fails)
        AgentExportFilePlan fp;
        fp.file_id = AgentExportFileId("");
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "test.md";
        fp.content = "content";
        fp.byte_size = 7;
        ASSERT_ERROR(fp.validateBasic());
    PASS()
}

void test_file_plan_unknown_kind() {
    TEST(file_plan_unknown_kind_fails)
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Unknown;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "test.md";
        fp.content = "content";
        fp.byte_size = 7;
        ASSERT_ERROR(fp.validateBasic());
    PASS()
}

void test_file_plan_unknown_ext() {
    TEST(file_plan_unknown_extension_fails)
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Unknown;
        fp.relative_path = "test.md";
        fp.content = "content";
        fp.byte_size = 7;
        ASSERT_ERROR(fp.validateBasic());
    PASS()
}

void test_file_plan_empty_path() {
    TEST(file_plan_empty_relative_path_fails)
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "";
        fp.content = "content";
        fp.byte_size = 7;
        ASSERT_ERROR(fp.validateBasic());
    PASS()
}

void test_file_plan_traversal() {
    TEST(file_plan_relative_path_traversal_fails)
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "../escape.md";
        fp.content = "content";
        fp.byte_size = 7;
        ASSERT_ERROR(fp.validateBasic());
    PASS()
}

void test_file_plan_backslash() {
    TEST(file_plan_relative_path_backslash_fails)
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "dir\\file.md";
        fp.content = "content";
        fp.byte_size = 7;
        ASSERT_ERROR(fp.validateBasic());
    PASS()
}

void test_file_plan_absolute() {
    TEST(file_plan_absolute_relative_path_fails)
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "/etc/passwd";
        fp.content = "content";
        fp.byte_size = 7;
        ASSERT_ERROR(fp.validateBasic());
    PASS()
}

void test_file_plan_manifest_empty() {
    TEST(file_plan_manifest_empty_content_fails)
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "manifest.md";
        fp.content = "";
        fp.byte_size = 0;
        ASSERT_ERROR(fp.validateBasic());
    PASS()
}

void test_file_plan_size_mismatch() {
    TEST(file_plan_byte_size_mismatch_fails)
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "manifest.md";
        fp.content = "hello";
        fp.byte_size = 999;
        ASSERT_ERROR(fp.validateBasic());
    PASS()
}

// --- WritePlan validation tests ---

void test_write_plan_empty_id() {
    TEST(write_plan_empty_write_id_fails)
        AgentExportWritePlan plan;
        plan.write_id = AgentExportWriteId("");
        plan.export_id = "exp001";
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "m.md";
        fp.content = "x";
        fp.byte_size = 1;
        plan.files.push_back(fp);
        plan.total_bytes = 1;
        ASSERT_ERROR(plan.validateBasic());
    PASS()
}

void test_write_plan_empty_export() {
    TEST(write_plan_empty_export_id_fails)
        AgentExportWritePlan plan;
        plan.write_id = makeAgentExportWriteId("e", 1);
        plan.export_id = "";
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "m.md";
        fp.content = "x";
        fp.byte_size = 1;
        plan.files.push_back(fp);
        plan.total_bytes = 1;
        plan.path_policy.root_dir = "/tmp/test";
        ASSERT_ERROR(plan.validateBasic());
    PASS()
}

void test_write_plan_empty_files() {
    TEST(write_plan_empty_files_fails)
        AgentExportWritePlan plan;
        plan.write_id = makeAgentExportWriteId("e", 0);
        plan.export_id = "exp001";
        plan.path_policy.root_dir = "/tmp/test";
        ASSERT_ERROR(plan.validateBasic());
    PASS()
}

void test_write_plan_exceed_count() {
    TEST(write_plan_exceeds_max_file_count_fails)
        AgentExportWritePlan plan;
        plan.write_id = makeAgentExportWriteId("e", 3);
        plan.export_id = "exp001";
        plan.path_policy.root_dir = "/tmp/test";
        plan.path_policy.max_file_count = 1;
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "m.md";
        fp.content = "x";
        fp.byte_size = 1;
        plan.files.push_back(fp);
        plan.files.push_back(fp);
        plan.total_bytes = 2;
        ASSERT_ERROR(plan.validateBasic());
    PASS()
}

void test_write_plan_dup_path() {
    TEST(write_plan_duplicate_relative_path_fails)
        AgentExportWritePlan plan;
        plan.write_id = makeAgentExportWriteId("e", 2);
        plan.export_id = "exp001";
        plan.path_policy.root_dir = "/tmp/test";
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "same.md";
        fp.content = "x";
        fp.byte_size = 1;
        plan.files.push_back(fp);
        fp.file_id = makeAgentExportFileId("e", "m", 1);
        plan.files.push_back(fp);
        plan.total_bytes = 2;
        ASSERT_ERROR(plan.validateBasic());
    PASS()
}

void test_write_plan_no_manifest() {
    TEST(write_plan_missing_manifest_fails)
        AgentExportWritePlan plan;
        plan.write_id = makeAgentExportWriteId("e", 1);
        plan.export_id = "exp001";
        plan.path_policy.root_dir = "/tmp/test";
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "c", 0);
        fp.kind = AgentExportFileKind::Chunk;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "chunk.md";
        fp.content = "x";
        fp.byte_size = 1;
        plan.files.push_back(fp);
        plan.total_bytes = 1;
        ASSERT_ERROR(plan.validateBasic());
    PASS()
}

void test_write_plan_total_mismatch() {
    TEST(write_plan_total_bytes_mismatch_fails)
        AgentExportWritePlan plan;
        plan.write_id = makeAgentExportWriteId("e", 1);
        plan.export_id = "exp001";
        plan.path_policy.root_dir = "/tmp/test";
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "m.md";
        fp.content = "x";
        fp.byte_size = 1;
        plan.files.push_back(fp);
        plan.total_bytes = 999;
        ASSERT_ERROR(plan.validateBasic());
    PASS()
}

void test_write_plan_exceed_total() {
    TEST(write_plan_exceeds_max_total_bytes_fails)
        AgentExportWritePlan plan;
        plan.write_id = makeAgentExportWriteId("e", 1);
        plan.export_id = "exp001";
        plan.path_policy.root_dir = "/tmp/test";
        plan.path_policy.max_total_bytes = 5;
        std::string big_content(10, 'x');
        AgentExportFilePlan fp;
        fp.file_id = makeAgentExportFileId("e", "m", 0);
        fp.kind = AgentExportFileKind::Manifest;
        fp.extension = AgentExportFileExtension::Md;
        fp.relative_path = "m.md";
        fp.content = big_content;
        fp.byte_size = 10;
        plan.files.push_back(fp);
        plan.total_bytes = 10;
        ASSERT_ERROR(plan.validateBasic());
    PASS()
}

// --- FileWriteResult validation tests ---

void test_file_write_result_written_errors() {
    TEST(file_write_result_written_with_errors_fails)
        AgentExportFileWriteResult fr;
        fr.file_id = makeAgentExportFileId("e", "m", 0);
        fr.kind = AgentExportFileKind::Manifest;
        fr.status = AgentExportWriteStatus::Written;
        fr.relative_path = "m.md";
        fr.error_keys.push_back("some_error");
        ASSERT_ERROR(fr.validateBasic());
    PASS()
}

void test_file_write_result_failed_no_errors() {
    TEST(file_write_result_failed_without_errors_fails)
        AgentExportFileWriteResult fr;
        fr.file_id = makeAgentExportFileId("e", "m", 0);
        fr.kind = AgentExportFileKind::Manifest;
        fr.status = AgentExportWriteStatus::Failed;
        fr.relative_path = "m.md";
        ASSERT_ERROR(fr.validateBasic());
    PASS()
}

// --- WriteResult validation tests ---

void test_write_result_written_errors() {
    TEST(write_result_written_with_errors_fails)
        AgentExportWriteResult wr;
        wr.write_id = makeAgentExportWriteId("e", 1);
        wr.status = AgentExportWriteStatus::Written;
        wr.root_dir = "/tmp";
        wr.error_keys.push_back("err");
        ASSERT_ERROR(wr.validateBasic());
    PASS()
}

void test_write_result_failed_no_errors() {
    TEST(write_result_failed_without_errors_fails)
        AgentExportWriteResult wr;
        wr.write_id = makeAgentExportWriteId("e", 1);
        wr.status = AgentExportWriteStatus::Failed;
        wr.root_dir = "/tmp";
        ASSERT_ERROR(wr.validateBasic());
    PASS()
}

void test_write_result_count_mismatch() {
    TEST(write_result_written_count_mismatch_fails)
        AgentExportWriteResult wr;
        wr.write_id = makeAgentExportWriteId("e", 1);
        wr.status = AgentExportWriteStatus::Written;
        wr.root_dir = "/tmp";
        wr.planned_file_count = 1;
        wr.written_file_count = 99;
        ASSERT_ERROR(wr.validateBasic());
    PASS()
}

// --- VerificationReport validation tests ---

void test_verify_passed_count() {
    TEST(verify_report_passed_file_count_mismatch_fails)
        AgentExportVerificationReport rpt;
        rpt.verify_id = makeAgentExportVerifyId("w");
        rpt.write_id = makeAgentExportWriteId("e", 1);
        rpt.status = AgentExportVerificationStatus::Passed;
        rpt.expected_file_count = 2;
        rpt.actual_file_count = 1;
        rpt.expected_total_bytes = 10;
        rpt.actual_total_bytes = 10;
        ASSERT_ERROR(rpt.validateBasic());
    PASS()
}

void test_verify_passed_bytes() {
    TEST(verify_report_passed_total_bytes_mismatch_fails)
        AgentExportVerificationReport rpt;
        rpt.verify_id = makeAgentExportVerifyId("w");
        rpt.write_id = makeAgentExportWriteId("e", 1);
        rpt.status = AgentExportVerificationStatus::Passed;
        rpt.expected_file_count = 1;
        rpt.actual_file_count = 1;
        rpt.expected_total_bytes = 10;
        rpt.actual_total_bytes = 5;
        ASSERT_ERROR(rpt.validateBasic());
    PASS()
}

void test_verify_dup_paths() {
    TEST(verify_report_duplicate_checked_paths_fails)
        AgentExportVerificationReport rpt;
        rpt.verify_id = makeAgentExportVerifyId("w");
        rpt.write_id = makeAgentExportWriteId("e", 1);
        rpt.status = AgentExportVerificationStatus::Passed;
        rpt.expected_file_count = 1;
        rpt.actual_file_count = 1;
        rpt.expected_total_bytes = 10;
        rpt.actual_total_bytes = 10;
        rpt.checked_relative_paths.push_back("a.md");
        rpt.checked_relative_paths.push_back("a.md");
        ASSERT_ERROR(rpt.validateBasic());
    PASS()
}

// --- Planner tests ---

void test_planner_plaintext() {
    TEST(planner_plaintext_txt)
        auto draft = makeValidDraft(AgentExportFormat::PlainText);
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy("/tmp/p12_test");
        req.base_name = "test_export";

        AgentExportWritePlanner planner;
        auto result = planner.buildPlan(req);
        ASSERT_OK(result);
        ASSERT_EQ(result.value().files.size(), 2u);
        ASSERT_EQ(result.value().files[0].extension, AgentExportFileExtension::Txt);
        ASSERT_EQ(result.value().files[1].extension, AgentExportFileExtension::Txt);
    PASS()
}

void test_planner_markdown() {
    TEST(planner_markdown_md)
        auto draft = makeValidDraft(AgentExportFormat::MarkdownLike);
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy("/tmp/p12_test");
        req.base_name = "test_export";

        AgentExportWritePlanner planner;
        auto result = planner.buildPlan(req);
        ASSERT_OK(result);
        ASSERT_EQ(result.value().files.size(), 2u);
        ASSERT_EQ(result.value().files[0].extension, AgentExportFileExtension::Md);
        ASSERT_EQ(result.value().files[1].extension, AgentExportFileExtension::Md);
    PASS()
}

void test_planner_protocol_text() {
    TEST(planner_protocol_text_txt)
        auto draft = makeValidDraft(AgentExportFormat::ProtocolText);
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy("/tmp/p12_test");
        req.base_name = "test_export";

        AgentExportWritePlanner planner;
        auto result = planner.buildPlan(req);
        ASSERT_OK(result);
        ASSERT_EQ(result.value().files[0].extension, AgentExportFileExtension::Txt);
    PASS()
}

void test_planner_manifest_name() {
    TEST(planner_manifest_name_stable)
        auto draft = makeValidDraft(AgentExportFormat::MarkdownLike);
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy("/tmp/p12_test");
        req.base_name = "my_export";

        AgentExportWritePlanner planner;
        auto r1 = planner.buildPlan(req);
        auto r2 = planner.buildPlan(req);
        ASSERT_OK(r1);
        ASSERT_OK(r2);
        ASSERT_EQ(r1.value().files[0].relative_path, r2.value().files[0].relative_path);
        ASSERT_EQ(r1.value().files[0].relative_path, "my_export_manifest.md");
    PASS()
}

void test_planner_chunk_name() {
    TEST(planner_chunk_name_by_index)
        AgentExportChunk c0;
        c0.chunk_id = "c0";
        c0.format = AgentExportFormat::MarkdownLike;
        c0.title_key = "t0";
        c0.payload = "content0";
        c0.item_count = 1;

        AgentExportChunk c1;
        c1.chunk_id = "c1";
        c1.format = AgentExportFormat::MarkdownLike;
        c1.title_key = "t1";
        c1.payload = "content1";
        c1.item_count = 1;

        AgentExportDraft draft;
        draft.manifest.manifest_id = "exp_multi";
        draft.manifest.format = AgentExportFormat::MarkdownLike;
        draft.manifest.chunk_count = 2;
        draft.manifest.total_item_count = 2;
        draft.manifest.chunk_ids.push_back("c0");
        draft.manifest.chunk_ids.push_back("c1");
        draft.chunks.push_back(c0);
        draft.chunks.push_back(c1);

        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy("/tmp/p12_test");
        req.base_name = "multi";

        AgentExportWritePlanner planner;
        auto result = planner.buildPlan(req);
        ASSERT_OK(result);
        ASSERT_EQ(result.value().files.size(), 3u);
        ASSERT_EQ(result.value().files[1].relative_path, "multi_chunk_0.md");
        ASSERT_EQ(result.value().files[2].relative_path, "multi_chunk_1.md");
    PASS()
}

void test_planner_base_name_traversal() {
    TEST(planner_base_name_traversal_fails)
        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy("/tmp/p12_test");
        req.base_name = "../escape";

        AgentExportWritePlanner planner;
        auto result = planner.buildPlan(req);
        ASSERT_ERROR(result);
    PASS()
}

void test_planner_base_name_space() {
    TEST(planner_base_name_space_fails)
        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy("/tmp/p12_test");
        req.base_name = "has space";

        AgentExportWritePlanner planner;
        auto result = planner.buildPlan(req);
        ASSERT_ERROR(result);
    PASS()
}

void test_planner_base_name_colon() {
    TEST(planner_base_name_colon_fails)
        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy("/tmp/p12_test");
        req.base_name = "has:colon";

        AgentExportWritePlanner planner;
        auto result = planner.buildPlan(req);
        ASSERT_ERROR(result);
    PASS()
}

// --- Write service tests ---

static std::string test_root = "/tmp/p12_unit_test_export";

static void cleanup_test_dir() {
    std::error_code ec;
    std::filesystem::remove_all(test_root, ec);
}

void test_write_creates_directory() {
    TEST(write_creates_directory)
        cleanup_test_dir();

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.base_name = "test";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        AgentLocalExportService service;
        auto result = service.write(plan.value());
        ASSERT_OK(result);
        ASSERT_EQ(result.value().status, AgentExportWriteStatus::Written);
        ASSERT_EQ(result.value().written_file_count, 2u);
        ASSERT_TRUE(std::filesystem::exists(test_root));

        cleanup_test_dir();
    PASS()
}

void test_write_no_create_dir() {
    TEST(write_no_create_dir_fails)
        cleanup_test_dir();

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.path_policy.allow_create_directories = false;
        req.base_name = "test";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        AgentLocalExportService service;
        auto result = service.write(plan.value());
        ASSERT_OK(result);
        ASSERT_EQ(result.value().status, AgentExportWriteStatus::Failed);

        cleanup_test_dir();
    PASS()
}

void test_write_no_overwrite() {
    TEST(write_no_overwrite_fails)
        cleanup_test_dir();
        std::filesystem::create_directories(test_root);

        // Create a pre-existing file
        std::ofstream pre(test_root + "/test_manifest.md");
        pre << "existing";
        pre.close();

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.path_policy.allow_overwrite = false;
        req.base_name = "test";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        AgentLocalExportService service;
        auto result = service.write(plan.value());
        ASSERT_OK(result);
        ASSERT_EQ(result.value().status, AgentExportWriteStatus::Failed);

        cleanup_test_dir();
    PASS()
}

void test_write_overwrite() {
    TEST(write_overwrite_succeeds)
        cleanup_test_dir();
        std::filesystem::create_directories(test_root);

        std::ofstream pre(test_root + "/test_manifest.md");
        pre << "existing";
        pre.close();

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.path_policy.allow_overwrite = true;
        req.base_name = "test";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        AgentLocalExportService service;
        auto result = service.write(plan.value());
        ASSERT_OK(result);
        ASSERT_EQ(result.value().status, AgentExportWriteStatus::Written);

        cleanup_test_dir();
    PASS()
}

void test_write_written_count() {
    TEST(write_written_count_correct)
        cleanup_test_dir();

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.base_name = "cnt";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        AgentLocalExportService service;
        auto result = service.write(plan.value());
        ASSERT_OK(result);
        ASSERT_EQ(result.value().written_file_count, plan.value().files.size());
        ASSERT_EQ(result.value().planned_file_count, plan.value().files.size());

        cleanup_test_dir();
    PASS()
}

void test_write_failed_has_errors() {
    TEST(write_failed_has_error_keys)
        cleanup_test_dir();

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.path_policy.allow_create_directories = false;
        req.base_name = "test";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        AgentLocalExportService service;
        auto result = service.write(plan.value());
        ASSERT_OK(result);
        ASSERT_EQ(result.value().status, AgentExportWriteStatus::Failed);
        ASSERT_TRUE(!result.value().error_keys.empty());

        cleanup_test_dir();
    PASS()
}

// --- Verifier tests ---

void test_verify_passed() {
    TEST(verify_passed_after_write)
        cleanup_test_dir();

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.base_name = "vtest";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        AgentLocalExportService service;
        auto write_result = service.write(plan.value());
        ASSERT_OK(write_result);
        ASSERT_EQ(write_result.value().status, AgentExportWriteStatus::Written);

        AgentExportVerifyRequest vreq;
        vreq.plan = plan.value();
        vreq.write_result = write_result.value();
        vreq.scan_content_for_forbidden_terms = true;

        AgentExportVerifier verifier;
        auto vresult = verifier.verify(vreq);
        ASSERT_OK(vresult);
        ASSERT_EQ(vresult.value().status, AgentExportVerificationStatus::Passed);
        ASSERT_EQ(vresult.value().expected_file_count, vresult.value().actual_file_count);
        ASSERT_EQ(vresult.value().expected_total_bytes, vresult.value().actual_total_bytes);

        cleanup_test_dir();
    PASS()
}

void test_verify_missing_file() {
    TEST(verify_missing_file_fails)
        cleanup_test_dir();
        std::filesystem::create_directories(test_root);

        // Write only one of the two files
        std::ofstream(test_root + "/vtest_manifest.md") << "manifest content here";

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.base_name = "vtest";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        // Fabricate a write result that claims both written
        AgentExportWriteResult wr;
        wr.write_id = plan.value().write_id;
        wr.status = AgentExportWriteStatus::Written;
        wr.root_dir = test_root;
        wr.planned_file_count = 2;
        wr.written_file_count = 2;
        wr.total_bytes = plan.value().total_bytes;
        for (const auto& f : plan.value().files) {
            AgentExportFileWriteResult fr;
            fr.file_id = f.file_id;
            fr.kind = f.kind;
            fr.status = AgentExportWriteStatus::Written;
            fr.relative_path = f.relative_path;
            fr.byte_size = f.byte_size;
            wr.files.push_back(fr);
        }

        AgentExportVerifyRequest vreq;
        vreq.plan = plan.value();
        vreq.write_result = wr;
        vreq.scan_content_for_forbidden_terms = false;

        AgentExportVerifier verifier;
        auto vresult = verifier.verify(vreq);
        ASSERT_OK(vresult);
        ASSERT_EQ(vresult.value().status, AgentExportVerificationStatus::Failed);

        cleanup_test_dir();
    PASS()
}

void test_verify_size_mismatch() {
    TEST(verify_size_mismatch_fails)
        cleanup_test_dir();
        std::filesystem::create_directories(test_root);

        // Write a file with wrong content
        std::ofstream(test_root + "/sz_manifest.md") << "short";

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.base_name = "sz";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        AgentExportWriteResult wr;
        wr.write_id = plan.value().write_id;
        wr.status = AgentExportWriteStatus::Written;
        wr.root_dir = test_root;
        wr.planned_file_count = 2;
        wr.written_file_count = 2;
        wr.total_bytes = plan.value().total_bytes;
        for (const auto& f : plan.value().files) {
            AgentExportFileWriteResult fr;
            fr.file_id = f.file_id;
            fr.kind = f.kind;
            fr.status = AgentExportWriteStatus::Written;
            fr.relative_path = f.relative_path;
            fr.byte_size = f.byte_size;
            wr.files.push_back(fr);
        }

        AgentExportVerifyRequest vreq;
        vreq.plan = plan.value();
        vreq.write_result = wr;
        vreq.scan_content_for_forbidden_terms = false;

        AgentExportVerifier verifier;
        auto vresult = verifier.verify(vreq);
        ASSERT_OK(vresult);
        ASSERT_EQ(vresult.value().status, AgentExportVerificationStatus::Failed);

        cleanup_test_dir();
    PASS()
}

void test_verify_forbidden_content() {
    TEST(verify_forbidden_content_fails)
        cleanup_test_dir();
        std::filesystem::create_directories(test_root);

        // Write files with forbidden content
        std::ofstream(test_root + "/fb_manifest.md") << "manifest data";
        std::ofstream(test_root + "/fb_chunk_0.md") << "This contains GameState secret";

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.base_name = "fb";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        // Build write result matching actual files
        AgentExportWriteResult wr;
        wr.write_id = plan.value().write_id;
        wr.status = AgentExportWriteStatus::Written;
        wr.root_dir = test_root;
        wr.planned_file_count = 2;
        wr.written_file_count = 2;
        wr.total_bytes = 0;
        for (const auto& f : plan.value().files) {
            std::filesystem::path target = std::filesystem::path(test_root) / f.relative_path;
            size_t actual_size = std::filesystem::file_size(target);
            AgentExportFileWriteResult fr;
            fr.file_id = f.file_id;
            fr.kind = f.kind;
            fr.status = AgentExportWriteStatus::Written;
            fr.relative_path = f.relative_path;
            fr.byte_size = actual_size;
            wr.files.push_back(fr);
            wr.total_bytes += actual_size;
        }

        AgentExportVerifyRequest vreq;
        vreq.plan = plan.value();
        vreq.write_result = wr;
        vreq.scan_content_for_forbidden_terms = true;

        AgentExportVerifier verifier;
        auto vresult = verifier.verify(vreq);
        ASSERT_OK(vresult);
        ASSERT_EQ(vresult.value().status, AgentExportVerificationStatus::Failed);
        ASSERT_TRUE(!vresult.value().error_keys.empty());

        cleanup_test_dir();
    PASS()
}

void test_verify_no_content_scan() {
    TEST(verify_no_content_scan)
        cleanup_test_dir();

        auto draft = makeValidDraft();
        AgentExportWritePlanRequest req;
        req.draft = draft;
        req.path_policy = makeValidPolicy(test_root);
        req.base_name = "ns";

        AgentExportWritePlanner planner;
        auto plan = planner.buildPlan(req);
        ASSERT_OK(plan);

        // Write files normally first
        AgentLocalExportService service;
        auto write_result = service.write(plan.value());
        ASSERT_OK(write_result);
        ASSERT_EQ(write_result.value().status, AgentExportWriteStatus::Written);

        // Overwrite chunk with forbidden content (same size)
        std::string original_payload = draft.chunks[0].payload;
        std::string forbidden_payload = "Contains GameState secret";
        // Pad or truncate to same size
        if (forbidden_payload.size() < original_payload.size()) {
            forbidden_payload.append(original_payload.size() - forbidden_payload.size(), ' ');
        } else {
            forbidden_payload.resize(original_payload.size());
        }
        std::ofstream(test_root + "/ns_chunk_0.md") << forbidden_payload;

        AgentExportVerifyRequest vreq;
        vreq.plan = plan.value();
        vreq.write_result = write_result.value();
        vreq.scan_content_for_forbidden_terms = false;

        AgentExportVerifier verifier;
        auto vresult = verifier.verify(vreq);
        ASSERT_OK(vresult);
        // Should pass because we're not scanning content
        ASSERT_EQ(vresult.value().status, AgentExportVerificationStatus::Passed);

        cleanup_test_dir();
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

    std::cout << "=== Agent Local Export Tests (P12) ===" << std::endl;

    run("smoke", []() {
        TEST(smoke)
            AgentExportFileKind k = AgentExportFileKind::Manifest;
            ASSERT_EQ(toString(k), "Manifest");
        PASS()
    });

    run("file_kind_roundtrip", test_file_kind_roundtrip);
    run("file_extension_roundtrip", test_file_extension_roundtrip);
    run("write_status_roundtrip", test_write_status_roundtrip);
    run("verification_status_roundtrip", test_verification_status_roundtrip);
    run("file_id_helper", test_file_id_helper);
    run("write_id_helper", test_write_id_helper);
    run("verify_id_helper", test_verify_id_helper);
    run("path_policy_empty_root_dir_fails", test_path_policy_empty_root);
    run("path_policy_root_dir_traversal_fails", test_path_policy_traversal);
    run("path_policy_max_file_count_fails", test_path_policy_max_file_count);
    run("path_policy_max_file_bytes_zero_fails", test_path_policy_max_file_bytes);
    run("path_policy_total_bytes_less_than_max_fails", test_path_policy_total_bytes);
    run("file_plan_empty_file_id_fails", test_file_plan_empty_id);
    run("file_plan_unknown_kind_fails", test_file_plan_unknown_kind);
    run("file_plan_unknown_extension_fails", test_file_plan_unknown_ext);
    run("file_plan_empty_relative_path_fails", test_file_plan_empty_path);
    run("file_plan_relative_path_traversal_fails", test_file_plan_traversal);
    run("file_plan_relative_path_backslash_fails", test_file_plan_backslash);
    run("file_plan_absolute_relative_path_fails", test_file_plan_absolute);
    run("file_plan_manifest_empty_content_fails", test_file_plan_manifest_empty);
    run("file_plan_byte_size_mismatch_fails", test_file_plan_size_mismatch);
    run("write_plan_empty_write_id_fails", test_write_plan_empty_id);
    run("write_plan_empty_export_id_fails", test_write_plan_empty_export);
    run("write_plan_empty_files_fails", test_write_plan_empty_files);
    run("write_plan_exceeds_max_file_count_fails", test_write_plan_exceed_count);
    run("write_plan_duplicate_relative_path_fails", test_write_plan_dup_path);
    run("write_plan_missing_manifest_fails", test_write_plan_no_manifest);
    run("write_plan_total_bytes_mismatch_fails", test_write_plan_total_mismatch);
    run("write_plan_exceeds_max_total_bytes_fails", test_write_plan_exceed_total);
    run("file_write_result_written_with_errors_fails", test_file_write_result_written_errors);
    run("file_write_result_failed_without_errors_fails", test_file_write_result_failed_no_errors);
    run("write_result_written_with_errors_fails", test_write_result_written_errors);
    run("write_result_failed_without_errors_fails", test_write_result_failed_no_errors);
    run("write_result_written_count_mismatch_fails", test_write_result_count_mismatch);
    run("verify_report_passed_file_count_mismatch_fails", test_verify_passed_count);
    run("verify_report_passed_total_bytes_mismatch_fails", test_verify_passed_bytes);
    run("verify_report_duplicate_checked_paths_fails", test_verify_dup_paths);
    run("planner_plaintext_txt", test_planner_plaintext);
    run("planner_markdown_md", test_planner_markdown);
    run("planner_protocol_text_txt", test_planner_protocol_text);
    run("planner_manifest_name_stable", test_planner_manifest_name);
    run("planner_chunk_name_by_index", test_planner_chunk_name);
    run("planner_base_name_traversal_fails", test_planner_base_name_traversal);
    run("planner_base_name_space_fails", test_planner_base_name_space);
    run("planner_base_name_colon_fails", test_planner_base_name_colon);
    run("write_creates_directory", test_write_creates_directory);
    run("write_no_create_dir_fails", test_write_no_create_dir);
    run("write_no_overwrite_fails", test_write_no_overwrite);
    run("write_overwrite_succeeds", test_write_overwrite);
    run("write_written_count_correct", test_write_written_count);
    run("write_failed_has_error_keys", test_write_failed_has_errors);
    run("verify_passed_after_write", test_verify_passed);
    run("verify_missing_file_fails", test_verify_missing_file);
    run("verify_size_mismatch_fails", test_verify_size_mismatch);
    run("verify_forbidden_content_fails", test_verify_forbidden_content);
    run("verify_no_content_scan", test_verify_no_content_scan);

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;

    cleanup_test_dir();
    return (pass_count == test_count) ? 0 : 1;
}
