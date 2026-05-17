#include "pathfinder/agent/debug_input.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace pathfinder::agent {

// --- Enum toString / fromString ---

std::string toString(AgentDebugInputKind kind) {
    switch (kind) {
        case AgentDebugInputKind::Unknown: return "Unknown";
        case AgentDebugInputKind::BuiltinFixtureBundle: return "BuiltinFixtureBundle";
        case AgentDebugInputKind::HistoryProjectionBundle: return "HistoryProjectionBundle";
        case AgentDebugInputKind::DebugReportBundle: return "DebugReportBundle";
        case AgentDebugInputKind::ExportDraftBundle: return "ExportDraftBundle";
        case AgentDebugInputKind::InMemoryTestBundle: return "InMemoryTestBundle";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugInputKind> agentDebugInputKindFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDebugInputKind>::ok(AgentDebugInputKind::Unknown);
    if (str == "BuiltinFixtureBundle") return pathfinder::foundation::Result<AgentDebugInputKind>::ok(AgentDebugInputKind::BuiltinFixtureBundle);
    if (str == "HistoryProjectionBundle") return pathfinder::foundation::Result<AgentDebugInputKind>::ok(AgentDebugInputKind::HistoryProjectionBundle);
    if (str == "DebugReportBundle") return pathfinder::foundation::Result<AgentDebugInputKind>::ok(AgentDebugInputKind::DebugReportBundle);
    if (str == "ExportDraftBundle") return pathfinder::foundation::Result<AgentDebugInputKind>::ok(AgentDebugInputKind::ExportDraftBundle);
    if (str == "InMemoryTestBundle") return pathfinder::foundation::Result<AgentDebugInputKind>::ok(AgentDebugInputKind::InMemoryTestBundle);
    return pathfinder::foundation::Result<AgentDebugInputKind>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent_debug_input_kind_unknown",
            "Unknown AgentDebugInputKind: " + str));
}

std::string toString(AgentDebugInputTrustLevel level) {
    switch (level) {
        case AgentDebugInputTrustLevel::Unknown: return "Unknown";
        case AgentDebugInputTrustLevel::Builtin: return "Builtin";
        case AgentDebugInputTrustLevel::GeneratedSafeDto: return "GeneratedSafeDto";
        case AgentDebugInputTrustLevel::TestOnly: return "TestOnly";
        case AgentDebugInputTrustLevel::ExternalUntrusted: return "ExternalUntrusted";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugInputTrustLevel> agentDebugInputTrustLevelFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDebugInputTrustLevel>::ok(AgentDebugInputTrustLevel::Unknown);
    if (str == "Builtin") return pathfinder::foundation::Result<AgentDebugInputTrustLevel>::ok(AgentDebugInputTrustLevel::Builtin);
    if (str == "GeneratedSafeDto") return pathfinder::foundation::Result<AgentDebugInputTrustLevel>::ok(AgentDebugInputTrustLevel::GeneratedSafeDto);
    if (str == "TestOnly") return pathfinder::foundation::Result<AgentDebugInputTrustLevel>::ok(AgentDebugInputTrustLevel::TestOnly);
    if (str == "ExternalUntrusted") return pathfinder::foundation::Result<AgentDebugInputTrustLevel>::ok(AgentDebugInputTrustLevel::ExternalUntrusted);
    return pathfinder::foundation::Result<AgentDebugInputTrustLevel>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent_debug_input_trust_level_unknown",
            "Unknown AgentDebugInputTrustLevel: " + str));
}

std::string toString(AgentDebugInputValidationStatus status) {
    switch (status) {
        case AgentDebugInputValidationStatus::Unknown: return "Unknown";
        case AgentDebugInputValidationStatus::Valid: return "Valid";
        case AgentDebugInputValidationStatus::ValidWithWarnings: return "ValidWithWarnings";
        case AgentDebugInputValidationStatus::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugInputValidationStatus> agentDebugInputValidationStatusFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDebugInputValidationStatus>::ok(AgentDebugInputValidationStatus::Unknown);
    if (str == "Valid") return pathfinder::foundation::Result<AgentDebugInputValidationStatus>::ok(AgentDebugInputValidationStatus::Valid);
    if (str == "ValidWithWarnings") return pathfinder::foundation::Result<AgentDebugInputValidationStatus>::ok(AgentDebugInputValidationStatus::ValidWithWarnings);
    if (str == "Invalid") return pathfinder::foundation::Result<AgentDebugInputValidationStatus>::ok(AgentDebugInputValidationStatus::Invalid);
    return pathfinder::foundation::Result<AgentDebugInputValidationStatus>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent_debug_input_validation_status_unknown",
            "Unknown AgentDebugInputValidationStatus: " + str));
}

std::string toString(AgentDebugInputCapability cap) {
    switch (cap) {
        case AgentDebugInputCapability::Unknown: return "Unknown";
        case AgentDebugInputCapability::CanBuildDebugReport: return "CanBuildDebugReport";
        case AgentDebugInputCapability::HasDiagnosticsSummary: return "HasDiagnosticsSummary";
        case AgentDebugInputCapability::HasExportDraft: return "HasExportDraft";
        case AgentDebugInputCapability::CanWriteThroughP12: return "CanWriteThroughP12";
        case AgentDebugInputCapability::TestOnly: return "TestOnly";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugInputCapability> agentDebugInputCapabilityFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDebugInputCapability>::ok(AgentDebugInputCapability::Unknown);
    if (str == "CanBuildDebugReport") return pathfinder::foundation::Result<AgentDebugInputCapability>::ok(AgentDebugInputCapability::CanBuildDebugReport);
    if (str == "HasDiagnosticsSummary") return pathfinder::foundation::Result<AgentDebugInputCapability>::ok(AgentDebugInputCapability::HasDiagnosticsSummary);
    if (str == "HasExportDraft") return pathfinder::foundation::Result<AgentDebugInputCapability>::ok(AgentDebugInputCapability::HasExportDraft);
    if (str == "CanWriteThroughP12") return pathfinder::foundation::Result<AgentDebugInputCapability>::ok(AgentDebugInputCapability::CanWriteThroughP12);
    if (str == "TestOnly") return pathfinder::foundation::Result<AgentDebugInputCapability>::ok(AgentDebugInputCapability::TestOnly);
    return pathfinder::foundation::Result<AgentDebugInputCapability>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent_debug_input_capability_unknown",
            "Unknown AgentDebugInputCapability: " + str));
}

std::string toString(AgentDebugInputRejectReason reason) {
    switch (reason) {
        case AgentDebugInputRejectReason::Unknown: return "Unknown";
        case AgentDebugInputRejectReason::EmptyInputId: return "EmptyInputId";
        case AgentDebugInputRejectReason::UnknownKind: return "UnknownKind";
        case AgentDebugInputRejectReason::UnknownTrustLevel: return "UnknownTrustLevel";
        case AgentDebugInputRejectReason::ExternalInputNotAllowed: return "ExternalInputNotAllowed";
        case AgentDebugInputRejectReason::MissingPayload: return "MissingPayload";
        case AgentDebugInputRejectReason::ConflictingPayloads: return "ConflictingPayloads";
        case AgentDebugInputRejectReason::InvalidManifest: return "InvalidManifest";
        case AgentDebugInputRejectReason::InvalidProjection: return "InvalidProjection";
        case AgentDebugInputRejectReason::InvalidReport: return "InvalidReport";
        case AgentDebugInputRejectReason::InvalidDiagnostics: return "InvalidDiagnostics";
        case AgentDebugInputRejectReason::InvalidExportDraft: return "InvalidExportDraft";
        case AgentDebugInputRejectReason::HiddenTruthDetected: return "HiddenTruthDetected";
        case AgentDebugInputRejectReason::RawStateDetected: return "RawStateDetected";
        case AgentDebugInputRejectReason::NotReportConvertible: return "NotReportConvertible";
        case AgentDebugInputRejectReason::TooManyItems: return "TooManyItems";
        case AgentDebugInputRejectReason::SchemaVersionUnsupported: return "SchemaVersionUnsupported";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugInputRejectReason> agentDebugInputRejectReasonFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::Unknown);
    if (str == "EmptyInputId") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::EmptyInputId);
    if (str == "UnknownKind") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::UnknownKind);
    if (str == "UnknownTrustLevel") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::UnknownTrustLevel);
    if (str == "ExternalInputNotAllowed") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::ExternalInputNotAllowed);
    if (str == "MissingPayload") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::MissingPayload);
    if (str == "ConflictingPayloads") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::ConflictingPayloads);
    if (str == "InvalidManifest") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::InvalidManifest);
    if (str == "InvalidProjection") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::InvalidProjection);
    if (str == "InvalidReport") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::InvalidReport);
    if (str == "InvalidDiagnostics") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::InvalidDiagnostics);
    if (str == "InvalidExportDraft") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::InvalidExportDraft);
    if (str == "HiddenTruthDetected") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::HiddenTruthDetected);
    if (str == "RawStateDetected") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::RawStateDetected);
    if (str == "NotReportConvertible") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::NotReportConvertible);
    if (str == "TooManyItems") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::TooManyItems);
    if (str == "SchemaVersionUnsupported") return pathfinder::foundation::Result<AgentDebugInputRejectReason>::ok(AgentDebugInputRejectReason::SchemaVersionUnsupported);
    return pathfinder::foundation::Result<AgentDebugInputRejectReason>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent_debug_input_reject_reason_unknown",
            "Unknown AgentDebugInputRejectReason: " + str));
}

// --- Forbidden keys ---

std::vector<std::string> agentDebugInputForbiddenKeys() {
    return {
        "edible_profile",
        "hunger_delta",
        "health_delta",
        "effect_kind",
        "objectdefinition",
        "gamestate",
        "agenttickrecord",
        "reward_value",
        "done =",
        "is_done",
        "savegame",
        "savemanager"
    };
}

// --- Helpers ---

namespace {

std::string toLower(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

bool containsForbiddenKey(const std::string& text, std::string& found_key) {
    std::string lower = toLower(text);
    for (const auto& key : agentDebugInputForbiddenKeys()) {
        if (lower.find(key) != std::string::npos) {
            found_key = key;
            return true;
        }
    }
    return false;
}

bool hasCapability(const std::vector<AgentDebugInputCapability>& caps, AgentDebugInputCapability target) {
    for (const auto& c : caps) {
        if (c == target) return true;
    }
    return false;
}

AgentDebugInputValidationIssue makeIssue(
    AgentDebugReportSeverity severity,
    AgentDebugInputRejectReason reason,
    const std::string& issue_key,
    const std::optional<std::string>& field_key = std::nullopt,
    const std::vector<std::string>& detail_keys = {}) {
    AgentDebugInputValidationIssue issue;
    issue.severity = severity;
    issue.reason = reason;
    issue.issue_key = issue_key;
    issue.field_key = field_key;
    issue.detail_keys = detail_keys;
    return issue;
}

} // anonymous namespace

// --- AgentDebugInputManifest::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugInputManifest::validateBasic() const {
    if (input_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "agent_debug_input_manifest_empty_input_id"));
    }
    if (kind == AgentDebugInputKind::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent_debug_input_manifest_unknown_kind"));
    }
    if (trust_level == AgentDebugInputTrustLevel::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent_debug_input_manifest_unknown_trust_level"));
    }
    if (schema_version != "agent_debug_input.v1") {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "agent_debug_input_manifest_schema_version_unsupported"));
    }
    if (requested_report_mode == AgentDebugReportMode::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent_debug_input_manifest_unknown_report_mode"));
    }
    for (const auto& cap : capabilities) {
        if (cap == AgentDebugInputCapability::Unknown) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_enum_unknown,
                    "agent_debug_input_manifest_unknown_capability"));
        }
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugInputSource::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugInputSource::validateBasic() const {
    auto mv = manifest.validateBasic();
    if (mv.is_error()) return mv;

    // Check payload presence based on kind
    size_t payload_count = 0;
    if (fixture.has_value()) ++payload_count;
    if (projection.has_value()) ++payload_count;
    if (report.has_value()) ++payload_count;
    if (diagnostics.has_value()) ++payload_count;
    if (export_draft.has_value()) ++payload_count;

    switch (manifest.kind) {
        case AgentDebugInputKind::BuiltinFixtureBundle:
            if (!fixture.has_value() || payload_count != 1) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input_source_builtin_fixture_only"));
            }
            break;
        case AgentDebugInputKind::HistoryProjectionBundle:
            if (!projection.has_value() || payload_count != 1) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input_source_projection_only"));
            }
            break;
        case AgentDebugInputKind::DebugReportBundle:
            if (!report.has_value()) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input_source_report_required"));
            }
            // diagnostics is optional, no other payloads allowed
            if (fixture.has_value() || projection.has_value() || export_draft.has_value()) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input_source_report_conflicting_payloads"));
            }
            break;
        case AgentDebugInputKind::ExportDraftBundle:
            if (!export_draft.has_value() || payload_count != 1) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input_source_export_draft_only"));
            }
            break;
        case AgentDebugInputKind::InMemoryTestBundle:
            // test_only must be true
            if (!manifest.test_only) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input_source_test_bundle_requires_test_only"));
            }
            // allowed: report or projection, not both with others
            if (!report.has_value() && !projection.has_value()) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input_source_test_bundle_needs_report_or_projection"));
            }
            if (fixture.has_value() || export_draft.has_value()) {
                return pathfinder::foundation::Result<void>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input_source_test_bundle_no_fixture_or_export"));
            }
            break;
        default:
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_enum_unknown,
                    "agent_debug_input_source_unknown_kind"));
    }

    // Validate contained DTOs
    if (projection.has_value()) {
        auto pv = projection->validateBasic();
        if (pv.is_error()) return pv;
    }
    if (report.has_value()) {
        auto rv = report->validateBasic();
        if (rv.is_error()) return rv;
    }
    if (diagnostics.has_value()) {
        auto dv = diagnostics->validateBasic();
        if (dv.is_error()) return dv;
    }
    if (export_draft.has_value()) {
        auto ev = export_draft->validateBasic();
        if (ev.is_error()) return ev;
    }

    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugInputValidationIssue::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugInputValidationIssue::validateBasic() const {
    if (severity == AgentDebugReportSeverity::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent_debug_input_issue_severity_unknown"));
    }
    if (reason == AgentDebugInputRejectReason::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent_debug_input_issue_reason_unknown"));
    }
    if (issue_key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "agent_debug_input_issue_empty_key"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugInputValidationReport::validateBasic ---

bool AgentDebugInputValidationReport::ok() const {
    return status == AgentDebugInputValidationStatus::Valid ||
           status == AgentDebugInputValidationStatus::ValidWithWarnings;
}

pathfinder::foundation::Result<void> AgentDebugInputValidationReport::validateBasic() const {
    if (input_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "agent_debug_input_report_empty_input_id"));
    }
    if (status == AgentDebugInputValidationStatus::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent_debug_input_report_status_unknown"));
    }
    if (kind == AgentDebugInputKind::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent_debug_input_report_kind_unknown"));
    }

    size_t error_count = 0;
    size_t warning_count = 0;
    for (const auto& issue : issues) {
        auto iv = issue.validateBasic();
        if (iv.is_error()) return iv;
        if (issue.severity == AgentDebugReportSeverity::Error) ++error_count;
        if (issue.severity == AgentDebugReportSeverity::Warning) ++warning_count;
    }

    if (status == AgentDebugInputValidationStatus::Valid) {
        if (error_count > 0) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "agent_debug_input_report_valid_has_errors"));
        }
    } else if (status == AgentDebugInputValidationStatus::ValidWithWarnings) {
        if (error_count > 0) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "agent_debug_input_report_valid_with_warnings_has_errors"));
        }
        if (warning_count == 0 && warning_keys.empty()) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "agent_debug_input_report_valid_with_warnings_no_warnings"));
        }
    } else if (status == AgentDebugInputValidationStatus::Invalid) {
        if (error_count == 0) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "agent_debug_input_report_invalid_no_errors"));
        }
    }

    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugInputAdapterOptions::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugInputAdapterOptions::validateBasic() const {
    if (report_mode == AgentDebugReportMode::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent_debug_input_adapter_options_unknown_report_mode"));
    }
    if (max_items == 0) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "agent_debug_input_adapter_options_max_items_zero"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugInputBundle::validateBasic ---

bool AgentDebugInputBundle::hasReport() const {
    return report.has_value();
}

bool AgentDebugInputBundle::hasExportDraft() const {
    return export_draft.has_value();
}

pathfinder::foundation::Result<void> AgentDebugInputBundle::validateBasic() const {
    auto mv = manifest.validateBasic();
    if (mv.is_error()) return mv;
    auto vv = validation.validateBasic();
    if (vv.is_error()) return vv;
    if (!validation.ok()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "agent_debug_input_bundle_validation_not_ok"));
    }
    if (report.has_value()) {
        auto rv = report->validateBasic();
        if (rv.is_error()) return rv;
    }
    if (diagnostics.has_value()) {
        auto dv = diagnostics->validateBasic();
        if (dv.is_error()) return dv;
    }
    if (export_draft.has_value()) {
        auto ev = export_draft->validateBasic();
        if (ev.is_error()) return ev;
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugInputValidator ---

pathfinder::foundation::Result<AgentDebugInputValidationReport> AgentDebugInputValidator::validate(
    const AgentDebugInputSource& source,
    const AgentDebugInputAdapterOptions& options) const {

    auto ov = options.validateBasic();
    if (ov.is_error()) return pathfinder::foundation::Result<AgentDebugInputValidationReport>::fail(ov.errors());

    AgentDebugInputValidationReport report;
    report.input_id = source.manifest.input_id;
    report.kind = source.manifest.kind;

    // Basic source validation
    auto sv = source.validateBasic();
    if (sv.is_error()) {
        report.status = AgentDebugInputValidationStatus::Invalid;
        report.issues.push_back(makeIssue(
            AgentDebugReportSeverity::Error,
            AgentDebugInputRejectReason::InvalidManifest,
            "agent_debug_input.invalid_manifest",
            std::nullopt,
            {sv.errors().empty() ? "source_validate_basic_failed" : sv.errors()[0].message_key}));
        return pathfinder::foundation::Result<AgentDebugInputValidationReport>::ok(std::move(report));
    }

    // Empty input id
    if (source.manifest.input_id.empty()) {
        report.issues.push_back(makeIssue(
            AgentDebugReportSeverity::Error,
            AgentDebugInputRejectReason::EmptyInputId,
            "agent_debug_input.empty_input_id"));
    }

    // Schema version
    if (source.manifest.schema_version != "agent_debug_input.v1") {
        report.issues.push_back(makeIssue(
            AgentDebugReportSeverity::Error,
            AgentDebugInputRejectReason::SchemaVersionUnsupported,
            "agent_debug_input.schema_version_unsupported",
            std::nullopt,
            {source.manifest.schema_version}));
    }

    // Trust level / external untrusted
    if (source.manifest.trust_level == AgentDebugInputTrustLevel::ExternalUntrusted && !options.allow_external_untrusted) {
        report.issues.push_back(makeIssue(
            AgentDebugReportSeverity::Error,
            AgentDebugInputRejectReason::ExternalInputNotAllowed,
            "agent_debug_input.external_input_not_allowed"));
    }

    // Test only
    if (source.manifest.test_only && !options.allow_test_only) {
        report.issues.push_back(makeIssue(
            AgentDebugReportSeverity::Error,
            AgentDebugInputRejectReason::ExternalInputNotAllowed,
            "agent_debug_input.test_only_not_allowed"));
    }

    // Kind-specific payload validation
    switch (source.manifest.kind) {
        case AgentDebugInputKind::BuiltinFixtureBundle:
            if (!source.fixture.has_value()) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::MissingPayload,
                    "agent_debug_input.missing_payload_fixture"));
            }
            break;
        case AgentDebugInputKind::HistoryProjectionBundle:
            if (!source.projection.has_value()) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::MissingPayload,
                    "agent_debug_input.missing_payload_projection"));
            } else {
                auto pv = source.projection->validateBasic();
                if (pv.is_error()) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::InvalidProjection,
                        "agent_debug_input.invalid_projection",
                        std::nullopt,
                        {pv.errors().empty() ? "projection_validate_failed" : pv.errors()[0].message_key}));
                }
            }
            break;
        case AgentDebugInputKind::DebugReportBundle:
            if (!source.report.has_value()) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::MissingPayload,
                    "agent_debug_input.missing_payload_report"));
            } else {
                auto rv = source.report->validateBasic();
                if (rv.is_error()) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::InvalidReport,
                        "agent_debug_input.invalid_report",
                        std::nullopt,
                        {rv.errors().empty() ? "report_validate_failed" : rv.errors()[0].message_key}));
                }
            }
            if (source.diagnostics.has_value()) {
                auto dv = source.diagnostics->validateBasic();
                if (dv.is_error()) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::InvalidDiagnostics,
                        "agent_debug_input.invalid_diagnostics",
                        std::nullopt,
                        {dv.errors().empty() ? "diagnostics_validate_failed" : dv.errors()[0].message_key}));
                }
            }
            break;
        case AgentDebugInputKind::ExportDraftBundle:
            if (!source.export_draft.has_value()) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::MissingPayload,
                    "agent_debug_input.missing_payload_export_draft"));
            } else {
                auto ev = source.export_draft->validateBasic();
                if (ev.is_error()) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::InvalidExportDraft,
                        "agent_debug_input.invalid_export_draft",
                        std::nullopt,
                        {ev.errors().empty() ? "export_draft_validate_failed" : ev.errors()[0].message_key}));
                }
            }
            break;
        case AgentDebugInputKind::InMemoryTestBundle:
            if (!source.manifest.test_only) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::ExternalInputNotAllowed,
                    "agent_debug_input.in_memory_test_bundle_requires_test_only"));
            }
            if (!options.allow_test_only) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::ExternalInputNotAllowed,
                    "agent_debug_input.in_memory_test_bundle_not_allowed"));
            }
            break;
        default:
            report.issues.push_back(makeIssue(
                AgentDebugReportSeverity::Error,
                AgentDebugInputRejectReason::UnknownKind,
                "agent_debug_input.unknown_kind"));
            break;
    }

    // Capability checks
    if (source.manifest.kind == AgentDebugInputKind::HistoryProjectionBundle) {
        if (!hasCapability(source.manifest.capabilities, AgentDebugInputCapability::CanBuildDebugReport)) {
            report.issues.push_back(makeIssue(
                AgentDebugReportSeverity::Error,
                AgentDebugInputRejectReason::InvalidManifest,
                "agent_debug_input.history_projection_missing_can_build_debug_report"));
        }
    } else if (source.manifest.kind == AgentDebugInputKind::DebugReportBundle) {
        if (!hasCapability(source.manifest.capabilities, AgentDebugInputCapability::CanBuildDebugReport)) {
            report.issues.push_back(makeIssue(
                AgentDebugReportSeverity::Error,
                AgentDebugInputRejectReason::InvalidManifest,
                "agent_debug_input.debug_report_missing_can_build_debug_report"));
        }
    } else if (source.manifest.kind == AgentDebugInputKind::ExportDraftBundle) {
        if (!hasCapability(source.manifest.capabilities, AgentDebugInputCapability::HasExportDraft)) {
            report.issues.push_back(makeIssue(
                AgentDebugReportSeverity::Error,
                AgentDebugInputRejectReason::InvalidManifest,
                "agent_debug_input.export_draft_missing_has_export_draft"));
        }
        if (hasCapability(source.manifest.capabilities, AgentDebugInputCapability::CanBuildDebugReport)) {
            report.issues.push_back(makeIssue(
                AgentDebugReportSeverity::Error,
                AgentDebugInputRejectReason::InvalidManifest,
                "agent_debug_input.export_draft_must_not_have_can_build_debug_report"));
        }
    }

    // Item count limit
    size_t item_count = 0;
    if (source.report.has_value()) item_count = source.report->items.size();
    else if (source.projection.has_value()) item_count = source.projection->items.size();
    else if (source.export_draft.has_value()) item_count = source.export_draft->manifest.total_item_count;

    report.checked_item_count = item_count;
    if (item_count > options.max_items) {
        report.issues.push_back(makeIssue(
            AgentDebugReportSeverity::Error,
            AgentDebugInputRejectReason::TooManyItems,
            "agent_debug_input.too_many_items",
            std::nullopt,
            {std::to_string(item_count) + " > " + std::to_string(options.max_items)}));
    }
    if (source.manifest.declared_item_count > 0 && source.manifest.declared_item_count != item_count) {
        report.warning_keys.push_back("declared_item_count_mismatch");
    }

    // Hidden truth / raw state scanning
    std::string found_key;

    // source_label
    if (containsForbiddenKey(source.manifest.source_label, found_key)) {
        report.issues.push_back(makeIssue(
            AgentDebugReportSeverity::Error,
            AgentDebugInputRejectReason::HiddenTruthDetected,
            "agent_debug_input.hidden_truth_detected",
            "manifest.source_label",
            {found_key}));
    }

    // manifest.warning_keys
    for (const auto& key : source.manifest.warning_keys) {
        if (containsForbiddenKey(key, found_key)) {
            report.issues.push_back(makeIssue(
                AgentDebugReportSeverity::Error,
                AgentDebugInputRejectReason::HiddenTruthDetected,
                "agent_debug_input.hidden_truth_detected",
                "manifest.warning_keys",
                {found_key}));
            break;
        }
    }

    // report items
    if (source.report.has_value()) {
        for (const auto& item : source.report->items) {
            for (const auto& key : item.summary_keys) {
                if (containsForbiddenKey(key, found_key)) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::HiddenTruthDetected,
                        "agent_debug_input.hidden_truth_detected",
                        "report.item.summary_keys",
                        {found_key}));
                    goto hidden_truth_done;
                }
            }
            for (const auto& key : item.reason_keys) {
                if (containsForbiddenKey(key, found_key)) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::HiddenTruthDetected,
                        "agent_debug_input.hidden_truth_detected",
                        "report.item.reason_keys",
                        {found_key}));
                    goto hidden_truth_done;
                }
            }
            for (const auto& key : item.phase_keys) {
                if (containsForbiddenKey(key, found_key)) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::HiddenTruthDetected,
                        "agent_debug_input.hidden_truth_detected",
                        "report.item.phase_keys",
                        {found_key}));
                    goto hidden_truth_done;
                }
            }
            for (const auto& key : item.warning_keys) {
                if (containsForbiddenKey(key, found_key)) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::HiddenTruthDetected,
                        "agent_debug_input.hidden_truth_detected",
                        "report.item.warning_keys",
                        {found_key}));
                    goto hidden_truth_done;
                }
            }
        }
    }

    // diagnostics issues
    if (source.diagnostics.has_value()) {
        for (const auto& issue : source.diagnostics->issues) {
            if (containsForbiddenKey(issue.issue_key, found_key)) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::HiddenTruthDetected,
                    "agent_debug_input.hidden_truth_detected",
                    "diagnostics.issue_key",
                    {found_key}));
                goto hidden_truth_done;
            }
            for (const auto& key : issue.detail_keys) {
                if (containsForbiddenKey(key, found_key)) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::HiddenTruthDetected,
                        "agent_debug_input.hidden_truth_detected",
                        "diagnostics.detail_keys",
                        {found_key}));
                    goto hidden_truth_done;
                }
            }
        }
    }

    // export draft
    if (source.export_draft.has_value()) {
        for (const auto& key : source.export_draft->manifest.warning_keys) {
            if (containsForbiddenKey(key, found_key)) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::HiddenTruthDetected,
                    "agent_debug_input.hidden_truth_detected",
                    "export_draft.manifest.warning_keys",
                    {found_key}));
                goto hidden_truth_done;
            }
        }
        for (const auto& chunk : source.export_draft->chunks) {
            if (containsForbiddenKey(chunk.title_key, found_key)) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::HiddenTruthDetected,
                    "agent_debug_input.hidden_truth_detected",
                    "export_draft.chunk.title_key",
                    {found_key}));
                goto hidden_truth_done;
            }
            if (containsForbiddenKey(chunk.payload, found_key)) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::HiddenTruthDetected,
                    "agent_debug_input.hidden_truth_detected",
                    "export_draft.chunk.payload",
                    {found_key}));
                goto hidden_truth_done;
            }
        }
    }

    // projection strings
    if (source.projection.has_value()) {
        for (const auto& item : source.projection->items) {
            if (containsForbiddenKey(item.runtime_status, found_key)) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::RawStateDetected,
                    "agent_debug_input.raw_state_detected",
                    "projection.runtime_status",
                    {found_key}));
                goto hidden_truth_done;
            }
            if (containsForbiddenKey(item.decision_status, found_key)) {
                report.issues.push_back(makeIssue(
                    AgentDebugReportSeverity::Error,
                    AgentDebugInputRejectReason::RawStateDetected,
                    "agent_debug_input.raw_state_detected",
                    "projection.decision_status",
                    {found_key}));
                goto hidden_truth_done;
            }
            for (const auto& key : item.phase_keys) {
                if (containsForbiddenKey(key, found_key)) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::RawStateDetected,
                        "agent_debug_input.raw_state_detected",
                        "projection.phase_keys",
                        {found_key}));
                    goto hidden_truth_done;
                }
            }
            for (const auto& key : item.reason_keys) {
                if (containsForbiddenKey(key, found_key)) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::RawStateDetected,
                        "agent_debug_input.raw_state_detected",
                        "projection.reason_keys",
                        {found_key}));
                    goto hidden_truth_done;
                }
            }
            for (const auto& key : item.warning_keys) {
                if (containsForbiddenKey(key, found_key)) {
                    report.issues.push_back(makeIssue(
                        AgentDebugReportSeverity::Error,
                        AgentDebugInputRejectReason::RawStateDetected,
                        "agent_debug_input.raw_state_detected",
                        "projection.warning_keys",
                        {found_key}));
                    goto hidden_truth_done;
                }
            }
        }
    }

hidden_truth_done:

    // Determine status
    size_t errors = 0;
    size_t warnings = 0;
    for (const auto& issue : report.issues) {
        if (issue.severity == AgentDebugReportSeverity::Error) ++errors;
        if (issue.severity == AgentDebugReportSeverity::Warning) ++warnings;
    }
    if (!report.warning_keys.empty()) ++warnings;

    if (errors > 0) {
        report.status = AgentDebugInputValidationStatus::Invalid;
    } else if (warnings > 0) {
        report.status = AgentDebugInputValidationStatus::ValidWithWarnings;
    } else {
        report.status = AgentDebugInputValidationStatus::Valid;
    }

    return pathfinder::foundation::Result<AgentDebugInputValidationReport>::ok(std::move(report));
}

// --- AgentDebugInputAdapter ---

pathfinder::foundation::Result<AgentDebugInputBundle> AgentDebugInputAdapter::adapt(
    const AgentDebugInputSource& source,
    const AgentDebugInputAdapterOptions& options) const {

    // Step 1: Validate
    AgentDebugInputValidator validator;
    auto vr = validator.validate(source, options);
    if (vr.is_error()) return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(vr.errors());

    if (!vr.value().ok()) {
        return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "agent_debug_input.adapter_validation_failed"));
    }

    // Step 2: Build bundle
    AgentDebugInputBundle bundle;
    bundle.manifest = source.manifest;
    bundle.validation = vr.value();

    switch (source.manifest.kind) {
        case AgentDebugInputKind::BuiltinFixtureBundle: {
            AgentDebugFixtureFactory factory;
            auto fb = factory.build(source.fixture.value());
            if (fb.is_error()) return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(fb.errors());
            bundle.report = fb.value().report;
            bundle.diagnostics = fb.value().diagnostics;
            break;
        }
        case AgentDebugInputKind::HistoryProjectionBundle: {
            AgentDebugReportBuildRequest req;
            req.projection = source.projection.value();
            req.mode = options.report_mode;
            req.include_trace_keys = options.include_trace_keys;
            AgentDebugReportBuilder builder;
            auto br = builder.build(req);
            if (br.is_error()) return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(br.errors());
            bundle.report = br.value();
            AgentDiagnosticsBuilder diag_builder;
            auto dr = diag_builder.build(br.value());
            if (dr.is_error()) return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(dr.errors());
            bundle.diagnostics = dr.value();
            break;
        }
        case AgentDebugInputKind::DebugReportBundle: {
            bundle.report = source.report.value();
            if (source.diagnostics.has_value()) {
                bundle.diagnostics = source.diagnostics.value();
            } else {
                AgentDiagnosticsBuilder diag_builder;
                auto dr = diag_builder.build(source.report.value());
                if (dr.is_error()) return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(dr.errors());
                bundle.diagnostics = dr.value();
            }
            break;
        }
        case AgentDebugInputKind::ExportDraftBundle: {
            if (!options.allow_export_draft_only) {
                return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input.adapter_export_draft_only_not_allowed"));
            }
            bundle.export_draft = source.export_draft.value();
            // No report for export draft
            break;
        }
        case AgentDebugInputKind::InMemoryTestBundle: {
            if (!options.allow_test_only) {
                return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(
                    pathfinder::foundation::makeError(
                        pathfinder::foundation::ErrorCode::validation_failed,
                        "agent_debug_input.adapter_test_only_not_allowed"));
            }
            if (source.report.has_value()) {
                bundle.report = source.report.value();
                if (source.diagnostics.has_value()) {
                    bundle.diagnostics = source.diagnostics.value();
                } else {
                    AgentDiagnosticsBuilder diag_builder;
                    auto dr = diag_builder.build(source.report.value());
                    if (dr.is_error()) return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(dr.errors());
                    bundle.diagnostics = dr.value();
                }
            } else if (source.projection.has_value()) {
                AgentDebugReportBuildRequest req;
                req.projection = source.projection.value();
                req.mode = options.report_mode;
                req.include_trace_keys = options.include_trace_keys;
                AgentDebugReportBuilder builder;
                auto br = builder.build(req);
                if (br.is_error()) return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(br.errors());
                bundle.report = br.value();
                AgentDiagnosticsBuilder diag_builder;
                auto dr = diag_builder.build(br.value());
                if (dr.is_error()) return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(dr.errors());
                bundle.diagnostics = dr.value();
            }
            break;
        }
        default:
            return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_enum_unknown,
                    "agent_debug_input.adapter_unknown_kind"));
    }

    auto bv = bundle.validateBasic();
    if (bv.is_error()) return pathfinder::foundation::Result<AgentDebugInputBundle>::fail(bv.errors());

    return pathfinder::foundation::Result<AgentDebugInputBundle>::ok(std::move(bundle));
}

// --- AgentDebugInputFactory ---

pathfinder::foundation::Result<AgentDebugInputSource> AgentDebugInputFactory::fromFixture(
    AgentDebugCliFixture fixture) {

    if (fixture == AgentDebugCliFixture::Unknown) {
        return pathfinder::foundation::Result<AgentDebugInputSource>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent_debug_input_factory_unknown_fixture"));
    }

    AgentDebugInputSource source;
    source.manifest.input_id = AgentDebugInputId("agent_debug_input_fixture_" + toString(fixture));
    source.manifest.kind = AgentDebugInputKind::BuiltinFixtureBundle;
    source.manifest.trust_level = AgentDebugInputTrustLevel::Builtin;
    source.manifest.source_label = "builtin_fixture_" + toString(fixture);
    source.manifest.schema_version = "agent_debug_input.v1";
    source.manifest.requested_report_mode = AgentDebugReportMode::Debug;
    source.manifest.include_trace_keys = true;
    source.manifest.test_only = false;
    source.manifest.capabilities.push_back(AgentDebugInputCapability::CanBuildDebugReport);
    source.fixture = fixture;
    return pathfinder::foundation::Result<AgentDebugInputSource>::ok(std::move(source));
}

pathfinder::foundation::Result<AgentDebugInputSource> AgentDebugInputFactory::fromProjection(
    const pathfinder::protocol::AgentHistoryProjection& projection,
    AgentDebugReportMode report_mode) {

    AgentDebugInputSource source;
    source.manifest.input_id = AgentDebugInputId(
        "agent_debug_input_projection_" + projection.query_id + "_" + std::to_string(projection.items.size()));
    source.manifest.kind = AgentDebugInputKind::HistoryProjectionBundle;
    source.manifest.trust_level = AgentDebugInputTrustLevel::GeneratedSafeDto;
    source.manifest.source_label = "projection_" + projection.query_id;
    source.manifest.schema_version = "agent_debug_input.v1";
    source.manifest.requested_report_mode = report_mode;
    source.manifest.include_trace_keys = (report_mode != AgentDebugReportMode::Public);
    source.manifest.test_only = false;
    source.manifest.declared_item_count = projection.items.size();
    source.manifest.capabilities.push_back(AgentDebugInputCapability::CanBuildDebugReport);
    source.projection = projection;
    return pathfinder::foundation::Result<AgentDebugInputSource>::ok(std::move(source));
}

pathfinder::foundation::Result<AgentDebugInputSource> AgentDebugInputFactory::fromReport(
    const AgentDebugReport& report,
    const std::optional<AgentDiagnosticsSummary>& diagnostics) {

    AgentDebugInputSource source;
    source.manifest.input_id = AgentDebugInputId(
        "agent_debug_input_report_" + report.report_id.value());
    source.manifest.kind = AgentDebugInputKind::DebugReportBundle;
    source.manifest.trust_level = AgentDebugInputTrustLevel::GeneratedSafeDto;
    source.manifest.source_label = "report_" + report.report_id.value();
    source.manifest.schema_version = "agent_debug_input.v1";
    source.manifest.requested_report_mode = report.mode;
    source.manifest.include_trace_keys = (report.mode != AgentDebugReportMode::Public);
    source.manifest.test_only = false;
    source.manifest.declared_item_count = report.items.size();
    source.manifest.capabilities.push_back(AgentDebugInputCapability::CanBuildDebugReport);
    if (diagnostics.has_value()) {
        source.manifest.capabilities.push_back(AgentDebugInputCapability::HasDiagnosticsSummary);
    }
    source.report = report;
    source.diagnostics = diagnostics;
    return pathfinder::foundation::Result<AgentDebugInputSource>::ok(std::move(source));
}

pathfinder::foundation::Result<AgentDebugInputSource> AgentDebugInputFactory::fromExportDraft(
    const AgentExportDraft& draft) {

    AgentDebugInputSource source;
    source.manifest.input_id = AgentDebugInputId(
        "agent_debug_input_export_" + draft.manifest.manifest_id);
    source.manifest.kind = AgentDebugInputKind::ExportDraftBundle;
    source.manifest.trust_level = AgentDebugInputTrustLevel::GeneratedSafeDto;
    source.manifest.source_label = "export_draft_" + draft.manifest.manifest_id;
    source.manifest.schema_version = "agent_debug_input.v1";
    source.manifest.requested_report_mode = AgentDebugReportMode::Public;
    source.manifest.include_trace_keys = false;
    source.manifest.test_only = false;
    source.manifest.declared_item_count = draft.manifest.total_item_count;
    source.manifest.capabilities.push_back(AgentDebugInputCapability::HasExportDraft);
    source.manifest.capabilities.push_back(AgentDebugInputCapability::CanWriteThroughP12);
    source.export_draft = draft;
    return pathfinder::foundation::Result<AgentDebugInputSource>::ok(std::move(source));
}

pathfinder::foundation::Result<AgentDebugInputSource> AgentDebugInputFactory::fromTestReport(
    const std::string& case_name,
    const AgentDebugReport& report) {

    AgentDebugInputSource source;
    source.manifest.input_id = AgentDebugInputId(
        "agent_debug_input_test_" + case_name);
    source.manifest.kind = AgentDebugInputKind::InMemoryTestBundle;
    source.manifest.trust_level = AgentDebugInputTrustLevel::TestOnly;
    source.manifest.source_label = "test_" + case_name;
    source.manifest.schema_version = "agent_debug_input.v1";
    source.manifest.requested_report_mode = report.mode;
    source.manifest.include_trace_keys = false;
    source.manifest.test_only = true;
    source.manifest.declared_item_count = report.items.size();
    source.manifest.capabilities.push_back(AgentDebugInputCapability::CanBuildDebugReport);
    source.manifest.capabilities.push_back(AgentDebugInputCapability::TestOnly);
    source.report = report;
    return pathfinder::foundation::Result<AgentDebugInputSource>::ok(std::move(source));
}

} // namespace pathfinder::agent
