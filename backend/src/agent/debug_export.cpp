#include "pathfinder/agent/debug_export.h"
#include <sstream>
#include <algorithm>

namespace pathfinder::agent {

// --- Enum toString / fromString ---

std::string toString(AgentExportFormat format) {
    switch (format) {
        case AgentExportFormat::Unknown: return "Unknown";
        case AgentExportFormat::PlainText: return "PlainText";
        case AgentExportFormat::MarkdownLike: return "MarkdownLike";
        case AgentExportFormat::ProtocolText: return "ProtocolText";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentExportFormat> agentExportFormatFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentExportFormat>::ok(AgentExportFormat::Unknown);
    if (str == "PlainText") return pathfinder::foundation::Result<AgentExportFormat>::ok(AgentExportFormat::PlainText);
    if (str == "MarkdownLike") return pathfinder::foundation::Result<AgentExportFormat>::ok(AgentExportFormat::MarkdownLike);
    if (str == "ProtocolText") return pathfinder::foundation::Result<AgentExportFormat>::ok(AgentExportFormat::ProtocolText);
    return pathfinder::foundation::Result<AgentExportFormat>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent_export_format_unknown",
            "Unknown AgentExportFormat: " + str));
}

// --- AgentExportChunk::validateBasic ---

pathfinder::foundation::Result<void> AgentExportChunk::validateBasic() const {
    if (chunk_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "export_chunk_missing_chunk_id"));
    }
    if (format == AgentExportFormat::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "export_chunk_format_unknown"));
    }
    if (title_key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "export_chunk_missing_title_key"));
    }
    if (payload.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "export_chunk_empty_payload"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentExportManifest::validateBasic ---

pathfinder::foundation::Result<void> AgentExportManifest::validateBasic() const {
    if (manifest_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "export_manifest_missing_manifest_id"));
    }
    if (format == AgentExportFormat::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "export_manifest_format_unknown"));
    }
    if (chunk_count != chunk_ids.size()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "export_manifest_chunk_count_mismatch"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentExportDraft::validateBasic ---

pathfinder::foundation::Result<void> AgentExportDraft::validateBasic() const {
    auto vr = manifest.validateBasic();
    if (vr.is_error()) return vr;

    if (manifest.chunk_count != chunks.size()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "export_draft_manifest_chunks_mismatch"));
    }

    // Verify chunk_ids match
    for (size_t i = 0; i < manifest.chunk_ids.size(); ++i) {
        if (i >= chunks.size() || manifest.chunk_ids[i] != chunks[i].chunk_id) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "export_draft_chunk_id_mismatch"));
        }
    }

    for (const auto& chunk : chunks) {
        auto cvr = chunk.validateBasic();
        if (cvr.is_error()) return cvr;
    }

    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentExportDraftBuildRequest::validateBasic ---

pathfinder::foundation::Result<void> AgentExportDraftBuildRequest::validateBasic() const {
    auto vr = report.validateBasic();
    if (vr.is_error()) return vr;
    if (format == AgentExportFormat::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "export_draft_build_request_format_unknown"));
    }
    if (max_items_per_chunk == 0) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "export_draft_max_items_per_chunk_zero"));
    }
    if (diagnostics) {
        auto dvr = diagnostics->validateBasic();
        if (dvr.is_error()) return dvr;
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- Helpers for payload generation ---

static std::string buildPlainTextPayload(const AgentDebugReport& report, size_t start, size_t end) {
    std::ostringstream oss;
    oss << "Debug Report: " << report.report_id.value() << "\n";
    oss << "Mode: " << toString(report.mode) << "\n";
    oss << "Items: " << start << "-" << end << " of " << report.total_items << "\n";
    for (size_t i = start; i < end && i < report.items.size(); ++i) {
        const auto& item = report.items[i];
        oss << "---\n";
        oss << "record_id: " << item.record_id << "\n";
        oss << "agent_id: " << item.agent_id << "\n";
        oss << "tick: " << item.tick << "\n";
        oss << "runtime: " << item.runtime_status << "\n";
        oss << "decision: " << item.decision_status << "\n";
        oss << "severity: " << toString(item.severity) << "\n";
        if (item.command_id) oss << "command_id: " << *item.command_id << "\n";
        if (item.replay_lock_status) oss << "replay_lock: " << *item.replay_lock_status << "\n";
        if (item.training_sample_status) oss << "training_sample: " << *item.training_sample_status << "\n";
    }
    return oss.str();
}

static std::string buildMarkdownLikePayload(const AgentDebugReport& report, size_t start, size_t end) {
    std::ostringstream oss;
    oss << "# Debug Report: " << report.report_id.value() << "\n\n";
    oss << "**Mode:** " << toString(report.mode) << "  \n";
    oss << "**Items:** " << start << "-" << end << " of " << report.total_items << "\n\n";
    oss << "## Summary\n\n";
    oss << "- Replay Locked: " << report.replay_locked_count << "\n";
    oss << "- Explained Only: " << report.explained_only_count << "\n";
    oss << "- Broken: " << report.broken_count << "\n";
    oss << "- No Decision: " << report.no_decision_count << "\n";
    oss << "- Skipped: " << report.skipped_count << "\n\n";
    oss << "## Items\n\n";
    for (size_t i = start; i < end && i < report.items.size(); ++i) {
        const auto& item = report.items[i];
        oss << "### " << item.record_id << "\n\n";
        oss << "- **Agent:** " << item.agent_id << "\n";
        oss << "- **Tick:** " << item.tick << "\n";
        oss << "- **Runtime:** " << item.runtime_status << "\n";
        oss << "- **Decision:** " << item.decision_status << "\n";
        oss << "- **Severity:** " << toString(item.severity) << "\n";
        if (item.command_id) oss << "- **Command:** " << *item.command_id << "\n";
        if (item.replay_lock_status) oss << "- **Replay Lock:** " << *item.replay_lock_status << "\n";
        if (item.training_sample_status) oss << "- **Training Sample:** " << *item.training_sample_status << "\n";
        oss << "\n";
    }
    return oss.str();
}

static std::string buildProtocolTextPayload(const AgentDebugReport& report, size_t start, size_t end) {
    std::ostringstream oss;
    oss << "[REPORT] id=" << report.report_id.value()
        << " mode=" << toString(report.mode)
        << " total=" << report.total_items << "\n";
    oss << "[COUNTS] locked=" << report.replay_locked_count
        << " explained=" << report.explained_only_count
        << " broken=" << report.broken_count
        << " no_decision=" << report.no_decision_count
        << " skipped=" << report.skipped_count << "\n";
    for (size_t i = start; i < end && i < report.items.size(); ++i) {
        const auto& item = report.items[i];
        oss << "[ITEM] rec=" << item.record_id
            << " agent=" << item.agent_id
            << " tick=" << item.tick
            << " runtime=" << item.runtime_status
            << " decision=" << item.decision_status
            << " sev=" << toString(item.severity);
        if (item.command_id) oss << " cmd=" << *item.command_id;
        if (item.replay_lock_status) oss << " lock=" << *item.replay_lock_status;
        oss << "\n";
    }
    return oss.str();
}

// --- AgentExportDraftBuilder::build ---

pathfinder::foundation::Result<AgentExportDraft> AgentExportDraftBuilder::build(
    const AgentExportDraftBuildRequest& request) const {
    auto vr = request.validateBasic();
    if (vr.is_error()) return pathfinder::foundation::Result<AgentExportDraft>::fail(vr.errors());

    AgentExportDraft draft;
    size_t total = request.report.items.size();
    size_t chunk_idx = 0;

    for (size_t start = 0; start < total; start += request.max_items_per_chunk) {
        size_t end = std::min(start + request.max_items_per_chunk, total);
        size_t count = end - start;

        AgentExportChunk chunk;
        chunk.chunk_id = request.report.report_id.value() + "_chunk_" + std::to_string(chunk_idx);
        chunk.format = request.format;
        chunk.title_key = "debug_report_chunk_" + std::to_string(chunk_idx);
        chunk.item_count = count;

        switch (request.format) {
            case AgentExportFormat::PlainText:
                chunk.payload = buildPlainTextPayload(request.report, start, end);
                break;
            case AgentExportFormat::MarkdownLike:
                chunk.payload = buildMarkdownLikePayload(request.report, start, end);
                break;
            case AgentExportFormat::ProtocolText:
                chunk.payload = buildProtocolTextPayload(request.report, start, end);
                break;
            default:
                return pathfinder::foundation::Result<AgentExportDraft>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_enum_unknown,
                        "export_draft_unsupported_format"));
        }

        draft.chunks.push_back(std::move(chunk));
        chunk_idx++;
    }

    // Build manifest
    draft.manifest.manifest_id = request.report.report_id.value() + "_manifest";
    draft.manifest.format = request.format;
    draft.manifest.chunk_count = draft.chunks.size();
    draft.manifest.total_item_count = total;
    for (const auto& chunk : draft.chunks) {
        draft.manifest.chunk_ids.push_back(chunk.chunk_id);
    }

    // Copy diagnostics warnings
    if (request.diagnostics) {
        for (const auto& issue : request.diagnostics->issues) {
            if (issue.severity == AgentDebugReportSeverity::Warning) {
                draft.manifest.warning_keys.push_back(issue.issue_key);
            }
        }
    }

    auto draft_vr = draft.validateBasic();
    if (draft_vr.is_error()) return pathfinder::foundation::Result<AgentExportDraft>::fail(draft_vr.errors());

    return pathfinder::foundation::Result<AgentExportDraft>::ok(std::move(draft));
}

} // namespace pathfinder::agent
