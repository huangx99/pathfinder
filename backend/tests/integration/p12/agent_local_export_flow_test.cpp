#include "pathfinder/agent/local_export.h"
#include "pathfinder/protocol/agent_projection.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>

using namespace pathfinder::agent;
using namespace pathfinder::protocol;
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

static const std::string test_root = "/tmp/p12_integration_flow_test";

static void cleanup() {
    std::error_code ec;
    std::filesystem::remove_all(test_root, ec);
}

// Build a P11 AgentDebugReport from scratch (no AgentHistoryProjection dependency)
static AgentDebugReport makeDebugReport() {
    AgentDebugReportItem item1;
    item1.record_id = "rec_001";
    item1.agent_id = "agent_alpha";
    item1.actor_id = "actor_1";
    item1.tick = 100;
    item1.runtime_status = "PipelineSucceeded";
    item1.decision_status = "Decided";
    item1.command_id = "cmd_001";
    item1.severity = AgentDebugReportSeverity::Info;
    item1.summary_keys.push_back("pipeline_succeeded");

    AgentDebugReportItem item2;
    item2.record_id = "rec_002";
    item2.agent_id = "agent_alpha";
    item2.actor_id = "actor_1";
    item2.tick = 101;
    item2.runtime_status = "NoDecision";
    item2.decision_status = "NoDecision";
    item2.severity = AgentDebugReportSeverity::Warning;
    item2.summary_keys.push_back("no_decision");

    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q001", 2);
    report.mode = AgentDebugReportMode::Debug;
    report.total_items = 2;
    report.replay_locked_count = 0;
    report.explained_only_count = 1;
    report.broken_count = 0;
    report.no_decision_count = 1;
    report.skipped_count = 0;
    report.truncated = false;
    report.items.push_back(item1);
    report.items.push_back(item2);
    return report;
}

static AgentDiagnosticsSummary makeDiagnostics() {
    AgentDiagnosticsIssue issue;
    issue.severity = AgentDebugReportSeverity::Warning;
    issue.issue_key = "no_decision_no_explanation";
    issue.record_id = "rec_002";
    issue.agent_id = "agent_alpha";

    AgentDiagnosticsSummary diag;
    diag.status = AgentDiagnosticsStatus::HasWarnings;
    diag.item_count = 2;
    diag.warning_count = 1;
    diag.error_count = 0;
    diag.issues.push_back(issue);
    return diag;
}

void test_p12_markdown_flow() {
    TEST(p12_agent_local_export_flow)
        cleanup();

        // Step 1: Build P11 debug report
        auto report = makeDebugReport();
        auto rv = report.validateBasic();
        ASSERT_OK(rv);

        // Step 2: Build diagnostics
        auto diag = makeDiagnostics();
        auto dv = diag.validateBasic();
        ASSERT_OK(dv);

        // Step 3: Build P11 export draft (MarkdownLike)
        AgentExportDraftBuildRequest export_req;
        export_req.report = report;
        export_req.diagnostics = diag;
        export_req.format = AgentExportFormat::MarkdownLike;
        export_req.max_items_per_chunk = 50;

        AgentExportDraftBuilder draft_builder;
        auto draft_result = draft_builder.build(export_req);
        ASSERT_OK(draft_result);

        auto& draft = draft_result.value();
        auto ddv = draft.validateBasic();
        ASSERT_OK(ddv);

        // Step 4: Build write plan
        AgentExportPathPolicy policy;
        policy.root_dir = test_root;
        policy.allow_create_directories = true;
        policy.allow_overwrite = false;

        AgentExportWritePlanRequest plan_req;
        plan_req.draft = draft;
        plan_req.path_policy = policy;
        plan_req.base_name = "agent_debug";

        AgentExportWritePlanner planner;
        auto plan_result = planner.buildPlan(plan_req);
        ASSERT_OK(plan_result);

        // Step 5: Write files
        AgentLocalExportService service;
        auto write_result = service.write(plan_result.value());
        ASSERT_OK(write_result);
        ASSERT_EQ(write_result.value().status, AgentExportWriteStatus::Written);

        // Verify files exist
        ASSERT_TRUE(std::filesystem::exists(test_root + "/agent_debug_manifest.md"));
        ASSERT_TRUE(std::filesystem::exists(test_root + "/agent_debug_chunk_0.md"));

        // Step 6: Verify export
        AgentExportVerifyRequest verify_req;
        verify_req.plan = plan_result.value();
        verify_req.write_result = write_result.value();
        verify_req.scan_content_for_forbidden_terms = true;

        AgentExportVerifier verifier;
        auto verify_result = verifier.verify(verify_req);
        ASSERT_OK(verify_result);
        ASSERT_EQ(verify_result.value().status, AgentExportVerificationStatus::Passed);

        // Step 7: Check file content does not contain hidden truth
        std::ifstream manifest_file(test_root + "/agent_debug_manifest.md");
        std::string manifest_content((std::istreambuf_iterator<char>(manifest_file)),
                                      std::istreambuf_iterator<char>());
        ASSERT_TRUE(manifest_content.find("GameState") == std::string::npos);
        ASSERT_TRUE(manifest_content.find("ObjectDefinition") == std::string::npos);
        ASSERT_TRUE(manifest_content.find("edible_profile") == std::string::npos);
        ASSERT_TRUE(manifest_content.find("reward_value") == std::string::npos);

        std::ifstream chunk_file(test_root + "/agent_debug_chunk_0.md");
        std::string chunk_content((std::istreambuf_iterator<char>(chunk_file)),
                                   std::istreambuf_iterator<char>());
        ASSERT_TRUE(chunk_content.find("GameState") == std::string::npos);
        ASSERT_TRUE(chunk_content.find("ObjectDefinition") == std::string::npos);

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

    std::cout << "=== P12 Integration: Export Flow ===" << std::endl;

    run("smoke", []() {
        TEST(smoke)
            AgentExportFileKind k = AgentExportFileKind::Manifest;
            ASSERT_EQ(toString(k), "Manifest");
        PASS()
    });

    run("p12_agent_local_export_flow", test_p12_markdown_flow);

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;

    cleanup();
    return (pass_count == test_count) ? 0 : 1;
}
