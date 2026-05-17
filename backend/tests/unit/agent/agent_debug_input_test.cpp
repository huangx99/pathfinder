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

// --- Enum roundtrip tests ---

void test_kind_roundtrip() {
    TEST(kind_roundtrip)
        ASSERT_EQ(toString(AgentDebugInputKind::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentDebugInputKind::BuiltinFixtureBundle), "BuiltinFixtureBundle");
        ASSERT_EQ(toString(AgentDebugInputKind::HistoryProjectionBundle), "HistoryProjectionBundle");
        ASSERT_EQ(toString(AgentDebugInputKind::DebugReportBundle), "DebugReportBundle");
        ASSERT_EQ(toString(AgentDebugInputKind::ExportDraftBundle), "ExportDraftBundle");
        ASSERT_EQ(toString(AgentDebugInputKind::InMemoryTestBundle), "InMemoryTestBundle");

        auto r1 = agentDebugInputKindFromString("BuiltinFixtureBundle");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentDebugInputKind::BuiltinFixtureBundle);

        auto r2 = agentDebugInputKindFromString("Bogus");
        ASSERT_ERROR(r2);
    PASS()
}

void test_trust_level_roundtrip() {
    TEST(trust_level_roundtrip)
        ASSERT_EQ(toString(AgentDebugInputTrustLevel::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentDebugInputTrustLevel::Builtin), "Builtin");
        ASSERT_EQ(toString(AgentDebugInputTrustLevel::GeneratedSafeDto), "GeneratedSafeDto");
        ASSERT_EQ(toString(AgentDebugInputTrustLevel::TestOnly), "TestOnly");
        ASSERT_EQ(toString(AgentDebugInputTrustLevel::ExternalUntrusted), "ExternalUntrusted");

        auto r1 = agentDebugInputTrustLevelFromString("TestOnly");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentDebugInputTrustLevel::TestOnly);

        auto r2 = agentDebugInputTrustLevelFromString("Bogus");
        ASSERT_ERROR(r2);
    PASS()
}

void test_validation_status_roundtrip() {
    TEST(validation_status_roundtrip)
        ASSERT_EQ(toString(AgentDebugInputValidationStatus::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentDebugInputValidationStatus::Valid), "Valid");
        ASSERT_EQ(toString(AgentDebugInputValidationStatus::ValidWithWarnings), "ValidWithWarnings");
        ASSERT_EQ(toString(AgentDebugInputValidationStatus::Invalid), "Invalid");

        auto r1 = agentDebugInputValidationStatusFromString("Valid");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentDebugInputValidationStatus::Valid);

        auto r2 = agentDebugInputValidationStatusFromString("Bogus");
        ASSERT_ERROR(r2);
    PASS()
}

void test_capability_roundtrip() {
    TEST(capability_roundtrip)
        ASSERT_EQ(toString(AgentDebugInputCapability::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentDebugInputCapability::CanBuildDebugReport), "CanBuildDebugReport");
        ASSERT_EQ(toString(AgentDebugInputCapability::HasDiagnosticsSummary), "HasDiagnosticsSummary");
        ASSERT_EQ(toString(AgentDebugInputCapability::HasExportDraft), "HasExportDraft");
        ASSERT_EQ(toString(AgentDebugInputCapability::CanWriteThroughP12), "CanWriteThroughP12");
        ASSERT_EQ(toString(AgentDebugInputCapability::TestOnly), "TestOnly");

        auto r1 = agentDebugInputCapabilityFromString("HasExportDraft");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentDebugInputCapability::HasExportDraft);

        auto r2 = agentDebugInputCapabilityFromString("Bogus");
        ASSERT_ERROR(r2);
    PASS()
}

void test_reject_reason_roundtrip() {
    TEST(reject_reason_roundtrip)
        ASSERT_EQ(toString(AgentDebugInputRejectReason::Unknown), "Unknown");
        ASSERT_EQ(toString(AgentDebugInputRejectReason::EmptyInputId), "EmptyInputId");
        ASSERT_EQ(toString(AgentDebugInputRejectReason::HiddenTruthDetected), "HiddenTruthDetected");

        auto r1 = agentDebugInputRejectReasonFromString("HiddenTruthDetected");
        ASSERT_OK(r1);
        ASSERT_EQ(r1.value(), AgentDebugInputRejectReason::HiddenTruthDetected);

        auto r2 = agentDebugInputRejectReasonFromString("Bogus");
        ASSERT_ERROR(r2);
    PASS()
}

// --- Factory tests ---

void test_factory_from_fixture() {
    TEST(factory_from_fixture)
        auto r = AgentDebugInputFactory::fromFixture(AgentDebugCliFixture::UnknownFruit);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().manifest.kind, AgentDebugInputKind::BuiltinFixtureBundle);
        ASSERT_EQ(r.value().manifest.trust_level, AgentDebugInputTrustLevel::Builtin);
        ASSERT_TRUE(r.value().manifest.input_id.value().find("agent_debug_input_fixture_UnknownFruit") != std::string::npos);
        ASSERT_TRUE(r.value().fixture.has_value());
        ASSERT_EQ(r.value().fixture.value(), AgentDebugCliFixture::UnknownFruit);
    PASS()
}

void test_factory_from_report() {
    TEST(factory_from_report)
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

        auto r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().manifest.kind, AgentDebugInputKind::DebugReportBundle);
        ASSERT_EQ(r.value().manifest.trust_level, AgentDebugInputTrustLevel::GeneratedSafeDto);
        bool has_cap = false;
        for (const auto& c : r.value().manifest.capabilities) {
            if (c == AgentDebugInputCapability::CanBuildDebugReport) { has_cap = true; break; }
        }
        ASSERT_TRUE(has_cap);
    PASS()
}

void test_factory_from_projection() {
    TEST(factory_from_projection)
        pathfinder::protocol::AgentHistoryProjection projection;
        projection.query_id = "q1";
        projection.total_matched = 1;
        projection.projection_mode = pathfinder::agent::AgentHistoryProjectionMode::Public;
        pathfinder::protocol::AgentHistoryItemProjection item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        projection.items.push_back(item);

        auto r = AgentDebugInputFactory::fromProjection(projection);
        ASSERT_OK(r);
        ASSERT_EQ(r.value().manifest.kind, AgentDebugInputKind::HistoryProjectionBundle);
        ASSERT_EQ(r.value().manifest.trust_level, AgentDebugInputTrustLevel::GeneratedSafeDto);
        bool has_cap = false;
        for (const auto& c : r.value().manifest.capabilities) {
            if (c == AgentDebugInputCapability::CanBuildDebugReport) { has_cap = true; break; }
        }
        ASSERT_TRUE(has_cap);
        ASSERT_TRUE(r.value().projection.has_value());
    PASS()
}

// --- Validator tests ---

void test_validator_valid_fixture() {
    TEST(validator_valid_fixture)
        auto factory_r = AgentDebugInputFactory::fromFixture(AgentDebugCliFixture::UnknownFruit);
        ASSERT_OK(factory_r);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(factory_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(vr.value().ok());
        ASSERT_EQ(vr.value().status, AgentDebugInputValidationStatus::Valid);
    PASS()
}

void test_validator_valid_report() {
    TEST(validator_valid_report)
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

        auto factory_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(factory_r);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(factory_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(vr.value().ok());
    PASS()
}

void test_validator_valid_projection() {
    TEST(validator_valid_projection)
        pathfinder::protocol::AgentHistoryProjection projection;
        projection.query_id = "q1";
        projection.total_matched = 1;
        projection.projection_mode = pathfinder::agent::AgentHistoryProjectionMode::Public;
        pathfinder::protocol::AgentHistoryItemProjection item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        projection.items.push_back(item);

        auto factory_r = AgentDebugInputFactory::fromProjection(projection);
        ASSERT_OK(factory_r);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(factory_r.value());
        ASSERT_OK(vr);
        ASSERT_TRUE(vr.value().ok());
    PASS()
}

void test_validator_rejects_unknown_kind() {
    TEST(validator_rejects_unknown_kind)
        AgentDebugInputSource source;
        source.manifest.input_id = AgentDebugInputId("test");
        source.manifest.kind = AgentDebugInputKind::Unknown;
        source.manifest.trust_level = AgentDebugInputTrustLevel::Builtin;
        source.manifest.schema_version = "agent_debug_input.v1";

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source);
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
        ASSERT_EQ(vr.value().status, AgentDebugInputValidationStatus::Invalid);
    PASS()
}

void test_validator_rejects_unknown_trust_level() {
    TEST(validator_rejects_unknown_trust_level)
        AgentDebugInputSource source;
        source.manifest.input_id = AgentDebugInputId("test");
        source.manifest.kind = AgentDebugInputKind::BuiltinFixtureBundle;
        source.manifest.trust_level = AgentDebugInputTrustLevel::Unknown;
        source.manifest.schema_version = "agent_debug_input.v1";
        source.fixture = AgentDebugCliFixture::UnknownFruit;

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source);
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
        ASSERT_EQ(vr.value().status, AgentDebugInputValidationStatus::Invalid);
    PASS()
}

void test_validator_rejects_external_untrusted() {
    TEST(validator_rejects_external_untrusted)
        AgentDebugInputSource source;
        source.manifest.input_id = AgentDebugInputId("test");
        source.manifest.kind = AgentDebugInputKind::DebugReportBundle;
        source.manifest.trust_level = AgentDebugInputTrustLevel::ExternalUntrusted;
        source.manifest.schema_version = "agent_debug_input.v1";
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
        source.report = report;
        source.manifest.capabilities.push_back(AgentDebugInputCapability::CanBuildDebugReport);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source);
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

void test_validator_rejects_missing_payload() {
    TEST(validator_rejects_missing_payload)
        AgentDebugInputSource source;
        source.manifest.input_id = AgentDebugInputId("test");
        source.manifest.kind = AgentDebugInputKind::BuiltinFixtureBundle;
        source.manifest.trust_level = AgentDebugInputTrustLevel::Builtin;
        source.manifest.schema_version = "agent_debug_input.v1";
        // No fixture payload

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source);
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
        ASSERT_EQ(vr.value().status, AgentDebugInputValidationStatus::Invalid);
    PASS()
}

void test_validator_rejects_conflicting_payloads() {
    TEST(validator_rejects_conflicting_payloads)
        AgentDebugInputSource source;
        source.manifest.input_id = AgentDebugInputId("test");
        source.manifest.kind = AgentDebugInputKind::DebugReportBundle;
        source.manifest.trust_level = AgentDebugInputTrustLevel::GeneratedSafeDto;
        source.manifest.schema_version = "agent_debug_input.v1";
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
        source.report = report;
        // Also add conflicting export_draft
        AgentExportDraft draft;
        draft.manifest.manifest_id = "m1";
        draft.manifest.format = AgentExportFormat::PlainText;
        draft.manifest.chunk_count = 0;
        draft.manifest.total_item_count = 0;
        source.export_draft = draft;
        source.manifest.capabilities.push_back(AgentDebugInputCapability::CanBuildDebugReport);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source);
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
    PASS()
}

void test_validator_rejects_hidden_truth() {
    TEST(validator_rejects_hidden_truth)
        AgentDebugInputSource source;
        source.manifest.input_id = AgentDebugInputId("test");
        source.manifest.kind = AgentDebugInputKind::DebugReportBundle;
        source.manifest.trust_level = AgentDebugInputTrustLevel::GeneratedSafeDto;
        source.manifest.schema_version = "agent_debug_input.v1";
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
        item.summary_keys.push_back("edible_profile"); // hidden truth
        report.items.push_back(item);
        source.report = report;
        source.manifest.capabilities.push_back(AgentDebugInputCapability::CanBuildDebugReport);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source);
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
        ASSERT_EQ(vr.value().status, AgentDebugInputValidationStatus::Invalid);
    PASS()
}

void test_validator_rejects_raw_state() {
    TEST(validator_rejects_raw_state)
        AgentDebugInputSource source;
        source.manifest.input_id = AgentDebugInputId("test");
        source.manifest.kind = AgentDebugInputKind::HistoryProjectionBundle;
        source.manifest.trust_level = AgentDebugInputTrustLevel::GeneratedSafeDto;
        source.manifest.schema_version = "agent_debug_input.v1";
        pathfinder::protocol::AgentHistoryProjection projection;
        projection.query_id = "q1";
        projection.total_matched = 1;
        pathfinder::protocol::AgentHistoryItemProjection item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "GameState"; // raw state
        item.decision_status = "Decided";
        projection.items.push_back(item);
        source.projection = projection;
        source.manifest.capabilities.push_back(AgentDebugInputCapability::CanBuildDebugReport);

        AgentDebugInputValidator validator;
        auto vr = validator.validate(source);
        ASSERT_OK(vr);
        ASSERT_TRUE(!vr.value().ok());
        ASSERT_EQ(vr.value().status, AgentDebugInputValidationStatus::Invalid);
    PASS()
}

// --- Adapter tests ---

void test_adapter_fixture_to_report() {
    TEST(adapter_fixture_to_report)
        auto factory_r = AgentDebugInputFactory::fromFixture(AgentDebugCliFixture::UnknownFruit);
        ASSERT_OK(factory_r);

        AgentDebugInputAdapter adapter;
        auto ar = adapter.adapt(factory_r.value());
        ASSERT_OK(ar);
        ASSERT_TRUE(ar.value().hasReport());
        ASSERT_TRUE(ar.value().diagnostics.has_value());
    PASS()
}

void test_adapter_report_passthrough() {
    TEST(adapter_report_passthrough)
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

        auto factory_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(factory_r);

        AgentDebugInputAdapter adapter;
        auto ar = adapter.adapt(factory_r.value());
        ASSERT_OK(ar);
        ASSERT_TRUE(ar.value().hasReport());
        ASSERT_TRUE(ar.value().diagnostics.has_value());
        ASSERT_EQ(ar.value().report->report_id.value(), report.report_id.value());
    PASS()
}

void test_adapter_report_adds_diagnostics() {
    TEST(adapter_report_adds_diagnostics)
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

        auto factory_r = AgentDebugInputFactory::fromReport(report);
        ASSERT_OK(factory_r);

        AgentDebugInputAdapter adapter;
        auto ar = adapter.adapt(factory_r.value());
        ASSERT_OK(ar);
        ASSERT_TRUE(ar.value().diagnostics.has_value());
    PASS()
}

void test_adapter_projection_to_report() {
    TEST(adapter_projection_to_report)
        pathfinder::protocol::AgentHistoryProjection projection;
        projection.query_id = "q1";
        projection.total_matched = 1;
        projection.projection_mode = pathfinder::agent::AgentHistoryProjectionMode::Public;
        pathfinder::protocol::AgentHistoryItemProjection item;
        item.record_id = "r1";
        item.agent_id = "a1";
        item.actor_id = "ac1";
        item.tick = 1;
        item.runtime_status = "ok";
        item.decision_status = "Decided";
        projection.items.push_back(item);

        auto factory_r = AgentDebugInputFactory::fromProjection(projection);
        ASSERT_OK(factory_r);

        AgentDebugInputAdapter adapter;
        auto ar = adapter.adapt(factory_r.value());
        ASSERT_OK(ar);
        ASSERT_TRUE(ar.value().hasReport());
        ASSERT_TRUE(ar.value().diagnostics.has_value());
    PASS()
}

void test_adapter_rejects_export_draft_only_by_default() {
    TEST(adapter_rejects_export_draft_only_by_default)
        AgentExportDraft draft;
        draft.manifest.manifest_id = "m1";
        draft.manifest.format = AgentExportFormat::PlainText;
        draft.manifest.chunk_count = 0;
        draft.manifest.total_item_count = 0;

        auto factory_r = AgentDebugInputFactory::fromExportDraft(draft);
        ASSERT_OK(factory_r);

        AgentDebugInputAdapter adapter;
        auto ar = adapter.adapt(factory_r.value());
        ASSERT_ERROR(ar);
    PASS()
}

void test_adapter_accepts_export_draft_only_when_allowed() {
    TEST(adapter_accepts_export_draft_only_when_allowed)
        AgentExportDraft draft;
        draft.manifest.manifest_id = "m1";
        draft.manifest.format = AgentExportFormat::PlainText;
        draft.manifest.chunk_count = 0;
        draft.manifest.total_item_count = 0;

        auto factory_r = AgentDebugInputFactory::fromExportDraft(draft);
        ASSERT_OK(factory_r);

        AgentDebugInputAdapterOptions options;
        options.allow_export_draft_only = true;
        AgentDebugInputAdapter adapter;
        auto ar = adapter.adapt(factory_r.value(), options);
        ASSERT_OK(ar);
        ASSERT_TRUE(!ar.value().hasReport());
        ASSERT_TRUE(ar.value().hasExportDraft());
    PASS()
}

// --- Bundle tests ---

void test_bundle_validation_report_rules() {
    TEST(bundle_validation_report_rules)
        AgentDebugInputValidationReport vreport;
        vreport.input_id = AgentDebugInputId("test");
        vreport.status = AgentDebugInputValidationStatus::Valid;
        vreport.kind = AgentDebugInputKind::BuiltinFixtureBundle;
        ASSERT_TRUE(vreport.ok());

        vreport.status = AgentDebugInputValidationStatus::ValidWithWarnings;
        vreport.warning_keys.push_back("w1");
        ASSERT_TRUE(vreport.ok());

        vreport.status = AgentDebugInputValidationStatus::Invalid;
        AgentDebugInputValidationIssue issue;
        issue.severity = AgentDebugReportSeverity::Error;
        issue.reason = AgentDebugInputRejectReason::Unknown;
        issue.issue_key = "test_issue";
        vreport.issues.push_back(issue);
        ASSERT_TRUE(!vreport.ok());
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

    std::cout << "=== Agent Debug Input Tests (P14) ===" << std::endl;

    run("smoke", []() {
        TEST(smoke)
            AgentDebugInputKind k = AgentDebugInputKind::BuiltinFixtureBundle;
            ASSERT_EQ(toString(k), "BuiltinFixtureBundle");
        PASS()
    });

    // Enum roundtrip
    run("kind_roundtrip", test_kind_roundtrip);
    run("trust_level_roundtrip", test_trust_level_roundtrip);
    run("validation_status_roundtrip", test_validation_status_roundtrip);
    run("capability_roundtrip", test_capability_roundtrip);
    run("reject_reason_roundtrip", test_reject_reason_roundtrip);

    // Factory
    run("factory_from_fixture", test_factory_from_fixture);
    run("factory_from_report", test_factory_from_report);
    run("factory_from_projection", test_factory_from_projection);

    // Validator
    run("validator_valid_fixture", test_validator_valid_fixture);
    run("validator_valid_report", test_validator_valid_report);
    run("validator_valid_projection", test_validator_valid_projection);
    run("validator_rejects_unknown_kind", test_validator_rejects_unknown_kind);
    run("validator_rejects_unknown_trust_level", test_validator_rejects_unknown_trust_level);
    run("validator_rejects_external_untrusted", test_validator_rejects_external_untrusted);
    run("validator_rejects_missing_payload", test_validator_rejects_missing_payload);
    run("validator_rejects_conflicting_payloads", test_validator_rejects_conflicting_payloads);
    run("validator_rejects_hidden_truth", test_validator_rejects_hidden_truth);
    run("validator_rejects_raw_state", test_validator_rejects_raw_state);

    // Adapter
    run("adapter_fixture_to_report", test_adapter_fixture_to_report);
    run("adapter_report_passthrough", test_adapter_report_passthrough);
    run("adapter_report_adds_diagnostics", test_adapter_report_adds_diagnostics);
    run("adapter_projection_to_report", test_adapter_projection_to_report);
    run("adapter_rejects_export_draft_only_by_default", test_adapter_rejects_export_draft_only_by_default);
    run("adapter_accepts_export_draft_only_when_allowed", test_adapter_accepts_export_draft_only_when_allowed);

    // Bundle
    run("bundle_validation_report_rules", test_bundle_validation_report_rules);

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;
    return (pass_count == test_count) ? 0 : 1;
}
