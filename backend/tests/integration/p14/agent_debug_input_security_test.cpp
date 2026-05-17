#include "pathfinder/agent/debug_input.h"
#include <cassert>
#include <iostream>
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

// source_label contains GameState -> fail
void test_source_label_gamestate() {
    TEST(source_label_gamestate)
        AgentDebugReport report;
        report.report_id = makeAgentDebugReportId("test", 1);
        report.mode = AgentDebugReportMode::Public;
        report.total_items = 1;
        AgentDebugReportItem item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        item.severity = AgentDebugReportSeverity::Info;
        report.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(source_r);
        source_r.value().manifest.source_label = "GameState"; // inject forbidden

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

// summary_keys contains edible_profile -> fail
void test_summary_keys_edible_profile() {
    TEST(summary_keys_edible_profile)
        AgentDebugReport report;
        report.report_id = makeAgentDebugReportId("test", 1);
        report.mode = AgentDebugReportMode::Public;
        report.total_items = 1;
        AgentDebugReportItem item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        item.severity = AgentDebugReportSeverity::Info;
        item.summary_keys.push_back("edible_profile");
        report.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(source_r);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

// reason_keys contains hunger_delta -> fail
void test_reason_keys_hunger_delta() {
    TEST(reason_keys_hunger_delta)
        AgentDebugReport report;
        report.report_id = makeAgentDebugReportId("test", 1);
        report.mode = AgentDebugReportMode::Debug;
        report.total_items = 1;
        AgentDebugReportItem item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        item.severity = AgentDebugReportSeverity::Info;
        item.reason_keys.push_back("hunger_delta");
        report.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(source_r);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

// phase_keys contains health_delta -> fail
void test_phase_keys_health_delta() {
    TEST(phase_keys_health_delta)
        AgentDebugReport report;
        report.report_id = makeAgentDebugReportId("test", 1);
        report.mode = AgentDebugReportMode::Debug;
        report.total_items = 1;
        AgentDebugReportItem item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        item.severity = AgentDebugReportSeverity::Info;
        item.phase_keys.push_back("health_delta");
        report.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(source_r);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

// warning_keys contains effect_kind -> fail
void test_warning_keys_effect_kind() {
    TEST(warning_keys_effect_kind)
        AgentDebugReport report;
        report.report_id = makeAgentDebugReportId("test", 1);
        report.mode = AgentDebugReportMode::Debug;
        report.total_items = 1;
        AgentDebugReportItem item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        item.severity = AgentDebugReportSeverity::Info;
        item.warning_keys.push_back("effect_kind");
        report.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(source_r);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

// export payload contains ObjectDefinition -> fail
void test_export_payload_object_definition() {
    TEST(export_payload_object_definition)
        AgentExportDraft draft;
        draft.manifest.manifest_id = "test_export";
        draft.manifest.format = AgentExportFormat::PlainText;
        draft.manifest.chunk_count = 1;
        draft.manifest.total_item_count = 1;
        AgentExportChunk chunk;
        chunk.chunk_id = "c1";
        chunk.format = AgentExportFormat::PlainText;
        chunk.title_key = "title";
        chunk.payload = "test ObjectDefinition payload";
        chunk.item_count = 1;
        draft.chunks.push_back(chunk);
        draft.manifest.chunk_ids.push_back("c1");

        auto source_r = AgentDebugInputFactory::fromExportDraft(draft);
        ASSERT_OK(source_r);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

// ExternalUntrusted -> fail
void test_external_untrusted() {
    TEST(external_untrusted)
        AgentDebugReport report;
        report.report_id = makeAgentDebugReportId("test", 1);
        report.mode = AgentDebugReportMode::Public;
        report.total_items = 1;
        AgentDebugReportItem item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        item.severity = AgentDebugReportSeverity::Info;
        report.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(source_r);
        source_r.value().manifest.trust_level = AgentDebugInputTrustLevel::ExternalUntrusted;

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

// test_only input with allow_test_only=false -> fail
void test_test_only_not_allowed() {
    TEST(test_only_not_allowed)
        AgentDebugReport report;
        report.report_id = makeAgentDebugReportId("test", 1);
        report.mode = AgentDebugReportMode::Public;
        report.total_items = 1;
        AgentDebugReportItem item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        item.severity = AgentDebugReportSeverity::Info;
        report.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromTestReport("test_case", report);
        ASSERT_OK(source_r);
        ASSERT_TRUE(source_r.value().manifest.test_only);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

// test_only input with allow_test_only=true -> success
void test_test_only_allowed() {
    TEST(test_only_allowed)
        AgentDebugReport report;
        report.report_id = makeAgentDebugReportId("test", 1);
        report.mode = AgentDebugReportMode::Public;
        report.total_items = 1;
        AgentDebugReportItem item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        item.severity = AgentDebugReportSeverity::Info;
        report.items.push_back(item);

        auto source_r = AgentDebugInputFactory::fromTestReport("test_case", report);
        ASSERT_OK(source_r);
        ASSERT_TRUE(source_r.value().manifest.test_only);

        AgentDebugInputAdapterOptions options;
        options.allow_test_only = true;
        AgentDebugInputValidator validator;
        auto vr = validator.validate(source_r.value(), options);
        ASSERT_OK(vr);
        ASSERT_TRUE(vr.value().ok());

        AgentDebugInputAdapter adapter;
        auto ar = adapter.adapt(source_r.value(), options);
        ASSERT_OK(ar);
        ASSERT_TRUE(ar.value().hasReport());
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

    std::cout << "=== P14 Agent Debug Input Security Tests ===" << std::endl;

    run("smoke", []() {
        TEST(smoke)
            auto keys = agentDebugInputForbiddenKeys();
            ASSERT_TRUE(!keys.empty());
            ASSERT_TRUE(keys[0] == "edible_profile");
        PASS()
    });

    run("source_label_gamestate", test_source_label_gamestate);
    run("summary_keys_edible_profile", test_summary_keys_edible_profile);
    run("reason_keys_hunger_delta", test_reason_keys_hunger_delta);
    run("phase_keys_health_delta", test_phase_keys_health_delta);
    run("warning_keys_effect_kind", test_warning_keys_effect_kind);
    run("export_payload_object_definition", test_export_payload_object_definition);
    run("external_untrusted", test_external_untrusted);
    run("test_only_not_allowed", test_test_only_not_allowed);
    run("test_only_allowed", test_test_only_allowed);

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;
    return (pass_count == test_count) ? 0 : 1;
}
