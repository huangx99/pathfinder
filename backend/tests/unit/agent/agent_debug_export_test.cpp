#include "pathfinder/agent/debug_export.h"
#include "pathfinder/agent/debug_report.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::protocol;

// --- Helper: make a valid AgentDebugReport ---
AgentDebugReport makeValidReport() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_001", 2);
    report.mode = AgentDebugReportMode::Debug;
    report.total_items = 2;

    AgentDebugReportItem item1;
    item1.record_id = "rec_001";
    item1.agent_id = "agent_001";
    item1.actor_id = "actor_001";
    item1.tick = 10;
    item1.runtime_status = "PipelineSucceeded";
    item1.decision_status = "PipelineSucceeded";
    item1.command_id = "cmd_001";
    item1.severity = AgentDebugReportSeverity::Info;
    item1.replay_lock_status = "Locked";
    report.items.push_back(item1);

    AgentDebugReportItem item2;
    item2.record_id = "rec_002";
    item2.agent_id = "agent_001";
    item2.actor_id = "actor_001";
    item2.tick = 11;
    item2.runtime_status = "PipelineSucceeded";
    item2.decision_status = "PipelineSucceeded";
    item2.command_id = "cmd_002";
    item2.severity = AgentDebugReportSeverity::Info;
    item2.replay_lock_status = "Locked";
    report.items.push_back(item2);

    report.replay_locked_count = 2;
    return report;
}

// === Enum Roundtrip Tests ===

void format_roundtrip() {
    assert(toString(AgentExportFormat::Unknown) == "Unknown");
    assert(toString(AgentExportFormat::PlainText) == "PlainText");
    assert(toString(AgentExportFormat::MarkdownLike) == "MarkdownLike");
    assert(toString(AgentExportFormat::ProtocolText) == "ProtocolText");

    auto r1 = agentExportFormatFromString("PlainText");
    assert(r1.is_ok() && r1.value() == AgentExportFormat::PlainText);
    auto r2 = agentExportFormatFromString("MarkdownLike");
    assert(r2.is_ok() && r2.value() == AgentExportFormat::MarkdownLike);
    auto r3 = agentExportFormatFromString("ProtocolText");
    assert(r3.is_ok() && r3.value() == AgentExportFormat::ProtocolText);
    auto r4 = agentExportFormatFromString("Unknown");
    assert(r4.is_ok() && r4.value() == AgentExportFormat::Unknown);
    auto r5 = agentExportFormatFromString("Invalid");
    assert(r5.is_error());
}

// === Chunk Validation Tests ===

void valid_chunk_passes() {
    AgentExportChunk chunk;
    chunk.chunk_id = "chunk_001";
    chunk.format = AgentExportFormat::PlainText;
    chunk.title_key = "title";
    chunk.payload = "some content";
    chunk.item_count = 1;
    assert(chunk.validateBasic().is_ok());
}

void empty_chunk_id_fails() {
    AgentExportChunk chunk;
    chunk.chunk_id = "";
    chunk.format = AgentExportFormat::PlainText;
    chunk.title_key = "title";
    chunk.payload = "content";
    assert(chunk.validateBasic().is_error());
}

void format_unknown_fails() {
    AgentExportChunk chunk;
    chunk.chunk_id = "chunk_001";
    chunk.format = AgentExportFormat::Unknown;
    chunk.title_key = "title";
    chunk.payload = "content";
    assert(chunk.validateBasic().is_error());
}

void empty_payload_fails() {
    AgentExportChunk chunk;
    chunk.chunk_id = "chunk_001";
    chunk.format = AgentExportFormat::PlainText;
    chunk.title_key = "title";
    chunk.payload = "";
    assert(chunk.validateBasic().is_error());
}

// === Manifest Validation Tests ===

void valid_manifest_passes() {
    AgentExportManifest manifest;
    manifest.manifest_id = "manifest_001";
    manifest.format = AgentExportFormat::PlainText;
    manifest.chunk_count = 1;
    manifest.chunk_ids.push_back("chunk_001");
    assert(manifest.validateBasic().is_ok());
}

void manifest_chunk_count_mismatch_fails() {
    AgentExportManifest manifest;
    manifest.manifest_id = "manifest_001";
    manifest.format = AgentExportFormat::PlainText;
    manifest.chunk_count = 2;
    manifest.chunk_ids.push_back("chunk_001");
    assert(manifest.validateBasic().is_error());
}

// === Draft Validation Tests ===

void valid_draft_passes() {
    AgentExportDraft draft;
    draft.manifest.manifest_id = "manifest_001";
    draft.manifest.format = AgentExportFormat::PlainText;
    draft.manifest.chunk_count = 1;
    draft.manifest.chunk_ids.push_back("chunk_001");

    AgentExportChunk chunk;
    chunk.chunk_id = "chunk_001";
    chunk.format = AgentExportFormat::PlainText;
    chunk.title_key = "title";
    chunk.payload = "content";
    chunk.item_count = 1;
    draft.chunks.push_back(chunk);

    assert(draft.validateBasic().is_ok());
}

void draft_manifest_mismatch_fails() {
    AgentExportDraft draft;
    draft.manifest.manifest_id = "manifest_001";
    draft.manifest.format = AgentExportFormat::PlainText;
    draft.manifest.chunk_count = 2;
    draft.manifest.chunk_ids.push_back("chunk_001");
    draft.manifest.chunk_ids.push_back("chunk_002");

    AgentExportChunk chunk;
    chunk.chunk_id = "chunk_001";
    chunk.format = AgentExportFormat::PlainText;
    chunk.title_key = "title";
    chunk.payload = "content";
    chunk.item_count = 1;
    draft.chunks.push_back(chunk);

    assert(draft.validateBasic().is_error());
}

// === Builder Tests ===

void builder_plaintext_export() {
    AgentExportDraftBuilder builder;
    AgentExportDraftBuildRequest req;
    req.report = makeValidReport();
    req.format = AgentExportFormat::PlainText;
    req.max_items_per_chunk = 50;

    auto result = builder.build(req);
    assert(result.is_ok());
    const auto& draft = result.value();
    assert(draft.chunks.size() == 1);
    assert(draft.chunks[0].format == AgentExportFormat::PlainText);
    assert(!draft.chunks[0].payload.empty());
    assert(draft.manifest.chunk_count == 1);
    assert(draft.validateBasic().is_ok());
}

void builder_markdown_export() {
    AgentExportDraftBuilder builder;
    AgentExportDraftBuildRequest req;
    req.report = makeValidReport();
    req.format = AgentExportFormat::MarkdownLike;
    req.max_items_per_chunk = 50;

    auto result = builder.build(req);
    assert(result.is_ok());
    const auto& draft = result.value();
    assert(draft.chunks[0].format == AgentExportFormat::MarkdownLike);
    assert(draft.chunks[0].payload.find("# Debug Report") != std::string::npos);
    assert(draft.validateBasic().is_ok());
}

void builder_protocol_text_export() {
    AgentExportDraftBuilder builder;
    AgentExportDraftBuildRequest req;
    req.report = makeValidReport();
    req.format = AgentExportFormat::ProtocolText;
    req.max_items_per_chunk = 50;

    auto result = builder.build(req);
    assert(result.is_ok());
    const auto& draft = result.value();
    assert(draft.chunks[0].format == AgentExportFormat::ProtocolText);
    assert(draft.chunks[0].payload.find("[REPORT]") != std::string::npos);
    assert(draft.validateBasic().is_ok());
}

void builder_max_items_zero_fails() {
    AgentExportDraftBuilder builder;
    AgentExportDraftBuildRequest req;
    req.report = makeValidReport();
    req.format = AgentExportFormat::PlainText;
    req.max_items_per_chunk = 0;

    auto result = builder.build(req);
    assert(result.is_error());
}

void builder_chunk_count_correct() {
    auto report = makeValidReport();
    // Add more items to force chunking
    for (int i = 3; i <= 5; ++i) {
        AgentDebugReportItem item;
        item.record_id = "rec_00" + std::to_string(i);
        item.agent_id = "agent_001";
        item.actor_id = "actor_001";
        item.tick = 10 + i;
        item.runtime_status = "PipelineSucceeded";
        item.decision_status = "PipelineSucceeded";
        item.command_id = "cmd_00" + std::to_string(i);
        item.severity = AgentDebugReportSeverity::Info;
        report.items.push_back(item);
    }
    report.total_items = 5;

    AgentExportDraftBuilder builder;
    AgentExportDraftBuildRequest req;
    req.report = report;
    req.format = AgentExportFormat::PlainText;
    req.max_items_per_chunk = 2;

    auto result = builder.build(req);
    assert(result.is_ok());
    // 5 items / 2 per chunk = 3 chunks
    assert(result.value().chunks.size() == 3);
    assert(result.value().manifest.chunk_count == 3);
    assert(result.value().validateBasic().is_ok());
}

void builder_manifest_warning_keys() {
    AgentDiagnosticsSummary diag;
    diag.status = AgentDiagnosticsStatus::HasWarnings;
    diag.item_count = 2;
    diag.warning_count = 1;

    AgentDiagnosticsIssue issue;
    issue.severity = AgentDebugReportSeverity::Warning;
    issue.issue_key = "no_decision_no_explanation";
    diag.issues.push_back(issue);

    AgentExportDraftBuilder builder;
    AgentExportDraftBuildRequest req;
    req.report = makeValidReport();
    req.format = AgentExportFormat::PlainText;
    req.diagnostics = diag;

    auto result = builder.build(req);
    assert(result.is_ok());
    assert(!result.value().manifest.warning_keys.empty());
    assert(result.value().manifest.warning_keys[0] == "no_decision_no_explanation");
}

void builder_payload_no_hidden_truth() {
    AgentExportDraftBuilder builder;
    AgentExportDraftBuildRequest req;
    req.report = makeValidReport();
    req.format = AgentExportFormat::PlainText;

    auto result = builder.build(req);
    assert(result.is_ok());
    // Payload should not contain hidden truth keywords
    for (const auto& chunk : result.value().chunks) {
        assert(chunk.payload.find("reward_value") == std::string::npos);
        assert(chunk.payload.find("done =") == std::string::npos);
        assert(chunk.payload.find("is_done") == std::string::npos);
    }
}

// === Test Main ===

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P11 DebugExport test binary loaded." << std::endl;
        return 0;
    }

    if (test_name == "format_roundtrip") { format_roundtrip(); return 0; }
    if (test_name == "valid_chunk_passes") { valid_chunk_passes(); return 0; }
    if (test_name == "empty_chunk_id_fails") { empty_chunk_id_fails(); return 0; }
    if (test_name == "format_unknown_fails") { format_unknown_fails(); return 0; }
    if (test_name == "empty_payload_fails") { empty_payload_fails(); return 0; }
    if (test_name == "valid_manifest_passes") { valid_manifest_passes(); return 0; }
    if (test_name == "manifest_chunk_count_mismatch_fails") { manifest_chunk_count_mismatch_fails(); return 0; }
    if (test_name == "valid_draft_passes") { valid_draft_passes(); return 0; }
    if (test_name == "draft_manifest_mismatch_fails") { draft_manifest_mismatch_fails(); return 0; }
    if (test_name == "builder_plaintext_export") { builder_plaintext_export(); return 0; }
    if (test_name == "builder_markdown_export") { builder_markdown_export(); return 0; }
    if (test_name == "builder_protocol_text_export") { builder_protocol_text_export(); return 0; }
    if (test_name == "builder_max_items_zero_fails") { builder_max_items_zero_fails(); return 0; }
    if (test_name == "builder_chunk_count_correct") { builder_chunk_count_correct(); return 0; }
    if (test_name == "builder_manifest_warning_keys") { builder_manifest_warning_keys(); return 0; }
    if (test_name == "builder_payload_no_hidden_truth") { builder_payload_no_hidden_truth(); return 0; }

    std::cerr << "Unknown test: " << test_name << std::endl;
    return 1;
}
