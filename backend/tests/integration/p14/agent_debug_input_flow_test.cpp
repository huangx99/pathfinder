#include "pathfinder/agent/debug_input.h"
#include "pathfinder/agent/debug_cli.h"
#include "pathfinder/agent/debug_export.h"
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

static const std::string test_root = "/tmp/p14_integration_flow_test";

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

// P13 unknown_fruit fixture via P14 adapter -> export markdown
void test_p13_fixture_via_p14_adapter() {
    TEST(p13_fixture_via_p14_adapter)
        cleanup();

        // Build input from P13 fixture
        auto source_r = AgentDebugInputFactory::fromFixture(AgentDebugCliFixture::UnknownFruit);
        ASSERT_OK(source_r);

        // Adapt through P14
        AgentDebugInputAdapter adapter;
        auto bundle_r = adapter.adapt(source_r.value());
        ASSERT_OK(bundle_r);
        ASSERT_TRUE(bundle_r.value().hasReport());
        ASSERT_TRUE(bundle_r.value().diagnostics.has_value());

        // Build export draft (P12)
        AgentExportDraftBuildRequest draft_request;
        draft_request.report = bundle_r.value().report.value();
        draft_request.diagnostics = bundle_r.value().diagnostics.value();
        draft_request.format = AgentExportFormat::MarkdownLike;
        draft_request.max_items_per_chunk = 50;

        AgentExportDraftBuilder draft_builder;
        auto draft_result = draft_builder.build(draft_request);
        ASSERT_OK(draft_result);

        // Build write plan (P12)
        AgentExportPathPolicy path_policy;
        path_policy.root_dir = test_root;
        path_policy.allow_overwrite = true;

        AgentExportWritePlanRequest plan_request;
        plan_request.draft = draft_result.value();
        plan_request.path_policy = path_policy;
        plan_request.base_name = "unknown_fruit";

        AgentExportWritePlanner planner;
        auto plan_result = planner.buildPlan(plan_request);
        ASSERT_OK(plan_result);

        // Write files
        AgentLocalExportService export_service;
        auto write_result = export_service.write(plan_result.value());
        ASSERT_OK(write_result);
        ASSERT_EQ(write_result.value().status, AgentExportWriteStatus::Written);

        // Verify files exist
        ASSERT_TRUE(std::filesystem::exists(test_root + "/unknown_fruit_manifest.md"));
        ASSERT_TRUE(std::filesystem::exists(test_root + "/unknown_fruit_chunk_0.md"));

        cleanup();
    PASS()
}

// P13 no_decision fixture via P14 adapter -> dry run
void test_p13_no_decision_dry_run() {
    TEST(p13_no_decision_dry_run)
        cleanup();

        auto source_r = AgentDebugInputFactory::fromFixture(AgentDebugCliFixture::NoDecision);
        ASSERT_OK(source_r);

        AgentDebugInputAdapter adapter;
        auto bundle_r = adapter.adapt(source_r.value());
        ASSERT_OK(bundle_r);
        ASSERT_TRUE(bundle_r.value().hasReport());

        // Build plan but don't write (dry-run equivalent)
        AgentExportDraftBuildRequest draft_request;
        draft_request.report = bundle_r.value().report.value();
        draft_request.diagnostics = bundle_r.value().diagnostics.value();
        draft_request.format = AgentExportFormat::PlainText;

        AgentExportDraftBuilder draft_builder;
        auto draft_result = draft_builder.build(draft_request);
        ASSERT_OK(draft_result);

        AgentExportPathPolicy path_policy;
        path_policy.root_dir = test_root;

        AgentExportWritePlanRequest plan_request;
        plan_request.draft = draft_result.value();
        plan_request.path_policy = path_policy;
        plan_request.base_name = "no_decision";

        AgentExportWritePlanner planner;
        auto plan_result = planner.buildPlan(plan_request);
        ASSERT_OK(plan_result);
        ASSERT_TRUE(plan_result.value().files.size() > 0);

        // Verify no files written
        ASSERT_TRUE(!std::filesystem::exists(test_root));

        cleanup();
    PASS()
}

// P11 report input via P14 adapter -> P12 write plan
void test_p11_report_via_p14_to_p12() {
    TEST(p11_report_via_p14_to_p12)
        cleanup();

        // Build a P11 report
        AgentDebugReport report;
        report.report_id = makeAgentDebugReportId("p11_test", 1);
        report.mode = AgentDebugReportMode::Public;
        report.total_items = 1;
        AgentDebugReportItem item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "PipelineSucceeded";
        item.decision_status = "Decided";
        item.severity = AgentDebugReportSeverity::Info;
        report.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(source_r);

        AgentDebugInputAdapter adapter;
        auto bundle_r = adapter.adapt(source_r.value());
        ASSERT_OK(bundle_r);
        ASSERT_TRUE(bundle_r.value().hasReport());

        // P12 export
        AgentExportDraftBuildRequest draft_request;
        draft_request.report = bundle_r.value().report.value();
        draft_request.diagnostics = bundle_r.value().diagnostics.value();
        draft_request.format = AgentExportFormat::PlainText;

        AgentExportDraftBuilder draft_builder;
        auto draft_result = draft_builder.build(draft_request);
        ASSERT_OK(draft_result);

        AgentExportPathPolicy path_policy;
        path_policy.root_dir = test_root;
        path_policy.allow_overwrite = true;

        AgentExportWritePlanRequest plan_request;
        plan_request.draft = draft_result.value();
        plan_request.path_policy = path_policy;
        plan_request.base_name = "p11_report";

        AgentExportWritePlanner planner;
        auto plan_result = planner.buildPlan(plan_request);
        ASSERT_OK(plan_result);
        ASSERT_TRUE(plan_result.value().files.size() >= 1);

        cleanup();
    PASS()
}

// P10 projection input via P14 adapter -> AgentDebugReport
void test_p10_projection_via_p14_to_report() {
    TEST(p10_projection_via_p14_to_report)
        pathfinder::protocol::AgentHistoryProjection projection;
        projection.query_id = "p10_test";
        projection.total_matched = 1;
        projection.projection_mode = pathfinder::agent::AgentHistoryProjectionMode::Public;
        pathfinder::protocol::AgentHistoryItemProjection item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "PipelineSucceeded";
        item.decision_status = "Decided";
        projection.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromProjection(projection);
        ASSERT_OK(source_r);

        AgentDebugInputAdapter adapter;
        auto bundle_r = adapter.adapt(source_r.value());
        ASSERT_OK(bundle_r);
        ASSERT_TRUE(bundle_r.value().hasReport());
        ASSERT_EQ(bundle_r.value().report->total_items, 1u);
        ASSERT_TRUE(bundle_r.value().diagnostics.has_value());
    PASS()
}

// ExportDraftBundle does not reverse-engineer report
void test_export_draft_no_reverse_report() {
    TEST(export_draft_no_reverse_report)
        AgentExportDraft draft;
        draft.manifest.manifest_id = "test_export";
        draft.manifest.format = AgentExportFormat::PlainText;
        draft.manifest.chunk_count = 1;
        draft.manifest.total_item_count = 1;
        AgentExportChunk chunk;
        chunk.chunk_id = "c1";
        chunk.format = AgentExportFormat::PlainText;
        chunk.title_key = "title";
        chunk.payload = "test payload";
        chunk.item_count = 1;
        draft.chunks.push_back(chunk);
        draft.manifest.chunk_ids.push_back("c1");

        auto source_r = AgentDebugInputFactory::fromExportDraft(draft);
        ASSERT_OK(source_r);

        AgentDebugInputAdapterOptions options;
        options.allow_export_draft_only = true;
        AgentDebugInputAdapter adapter;
        auto bundle_r = adapter.adapt(source_r.value(), options);
        ASSERT_OK(bundle_r);
        ASSERT_TRUE(!bundle_r.value().hasReport());
        ASSERT_TRUE(bundle_r.value().hasExportDraft());
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

    std::cout << "=== P14 Agent Debug Input Flow Integration Tests ===" << std::endl;

    run("smoke", []() {
        TEST(smoke)
            AgentDebugInputKind k = AgentDebugInputKind::DebugReportBundle;
            ASSERT_EQ(toString(k), "DebugReportBundle");
        PASS()
    });

    run("p13_fixture_via_p14_adapter", test_p13_fixture_via_p14_adapter);
    run("p13_no_decision_dry_run", test_p13_no_decision_dry_run);
    run("p11_report_via_p14_to_p12", test_p11_report_via_p14_to_p12);
    run("p10_projection_via_p14_to_report", test_p10_projection_via_p14_to_report);
    run("export_draft_no_reverse_report", test_export_draft_no_reverse_report);

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;

    cleanup();
    return (pass_count == test_count) ? 0 : 1;
}
