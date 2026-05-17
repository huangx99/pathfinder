#pragma once

#include "pathfinder/agent/debug_cli.h"
#include "pathfinder/agent/debug_report.h"
#include "pathfinder/agent/debug_export.h"
#include "pathfinder/protocol/agent_projection.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>
#include <optional>
#include <cstddef>

namespace pathfinder::agent {

// --- ID Tag Types ---

struct AgentDebugInputIdTag {};
using AgentDebugInputId = pathfinder::foundation::StrongId<AgentDebugInputIdTag>;

// --- Enums ---

enum class AgentDebugInputKind {
    Unknown,
    BuiltinFixtureBundle,
    HistoryProjectionBundle,
    DebugReportBundle,
    ExportDraftBundle,
    InMemoryTestBundle
};

std::string toString(AgentDebugInputKind kind);
pathfinder::foundation::Result<AgentDebugInputKind> agentDebugInputKindFromString(const std::string& str);

enum class AgentDebugInputTrustLevel {
    Unknown,
    Builtin,
    GeneratedSafeDto,
    TestOnly,
    ExternalUntrusted
};

std::string toString(AgentDebugInputTrustLevel level);
pathfinder::foundation::Result<AgentDebugInputTrustLevel> agentDebugInputTrustLevelFromString(const std::string& str);

enum class AgentDebugInputValidationStatus {
    Unknown,
    Valid,
    ValidWithWarnings,
    Invalid
};

std::string toString(AgentDebugInputValidationStatus status);
pathfinder::foundation::Result<AgentDebugInputValidationStatus> agentDebugInputValidationStatusFromString(const std::string& str);

enum class AgentDebugInputCapability {
    Unknown,
    CanBuildDebugReport,
    HasDiagnosticsSummary,
    HasExportDraft,
    CanWriteThroughP12,
    TestOnly
};

std::string toString(AgentDebugInputCapability cap);
pathfinder::foundation::Result<AgentDebugInputCapability> agentDebugInputCapabilityFromString(const std::string& str);

enum class AgentDebugInputRejectReason {
    Unknown,
    EmptyInputId,
    UnknownKind,
    UnknownTrustLevel,
    ExternalInputNotAllowed,
    MissingPayload,
    ConflictingPayloads,
    InvalidManifest,
    InvalidProjection,
    InvalidReport,
    InvalidDiagnostics,
    InvalidExportDraft,
    HiddenTruthDetected,
    RawStateDetected,
    NotReportConvertible,
    TooManyItems,
    SchemaVersionUnsupported
};

std::string toString(AgentDebugInputRejectReason reason);
pathfinder::foundation::Result<AgentDebugInputRejectReason> agentDebugInputRejectReasonFromString(const std::string& str);

// --- Forbidden keys ---

std::vector<std::string> agentDebugInputForbiddenKeys();

// --- Data Contracts ---

struct AgentDebugInputManifest {
    AgentDebugInputId input_id;
    AgentDebugInputKind kind = AgentDebugInputKind::Unknown;
    AgentDebugInputTrustLevel trust_level = AgentDebugInputTrustLevel::Unknown;
    std::string source_label;
    std::string schema_version = "agent_debug_input.v1";
    AgentDebugReportMode requested_report_mode = AgentDebugReportMode::Public;
    bool include_trace_keys = false;
    bool test_only = false;
    size_t declared_item_count = 0;
    std::vector<AgentDebugInputCapability> capabilities;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentDebugInputSource {
    AgentDebugInputManifest manifest;
    std::optional<AgentDebugCliFixture> fixture;
    std::optional<pathfinder::protocol::AgentHistoryProjection> projection;
    std::optional<AgentDebugReport> report;
    std::optional<AgentDiagnosticsSummary> diagnostics;
    std::optional<AgentExportDraft> export_draft;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentDebugInputValidationIssue {
    AgentDebugReportSeverity severity = AgentDebugReportSeverity::Unknown;
    AgentDebugInputRejectReason reason = AgentDebugInputRejectReason::Unknown;
    std::string issue_key;
    std::optional<std::string> field_key;
    std::vector<std::string> detail_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentDebugInputValidationReport {
    AgentDebugInputId input_id;
    AgentDebugInputValidationStatus status = AgentDebugInputValidationStatus::Unknown;
    AgentDebugInputKind kind = AgentDebugInputKind::Unknown;
    size_t checked_item_count = 0;
    std::vector<std::string> checked_keys;
    std::vector<std::string> warning_keys;
    std::vector<AgentDebugInputValidationIssue> issues;

    bool ok() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentDebugInputAdapterOptions {
    AgentDebugReportMode report_mode = AgentDebugReportMode::Public;
    bool include_trace_keys = false;
    bool allow_test_only = false;
    bool allow_external_untrusted = false;
    bool allow_export_draft_only = false;
    size_t max_items = 1000;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentDebugInputBundle {
    AgentDebugInputManifest manifest;
    AgentDebugInputValidationReport validation;
    std::optional<AgentDebugReport> report;
    std::optional<AgentDiagnosticsSummary> diagnostics;
    std::optional<AgentExportDraft> export_draft;

    bool hasReport() const;
    bool hasExportDraft() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- Services ---

class AgentDebugInputValidator {
public:
    pathfinder::foundation::Result<AgentDebugInputValidationReport> validate(
        const AgentDebugInputSource& source,
        const AgentDebugInputAdapterOptions& options = {}) const;
};

class AgentDebugInputAdapter {
public:
    pathfinder::foundation::Result<AgentDebugInputBundle> adapt(
        const AgentDebugInputSource& source,
        const AgentDebugInputAdapterOptions& options = {}) const;
};

class AgentDebugInputFactory {
public:
    static pathfinder::foundation::Result<AgentDebugInputSource> fromFixture(
        AgentDebugCliFixture fixture);

    static pathfinder::foundation::Result<AgentDebugInputSource> fromProjection(
        const pathfinder::protocol::AgentHistoryProjection& projection,
        AgentDebugReportMode report_mode = AgentDebugReportMode::Public);

    static pathfinder::foundation::Result<AgentDebugInputSource> fromReport(
        const AgentDebugReport& report,
        const std::optional<AgentDiagnosticsSummary>& diagnostics = std::nullopt);

    static pathfinder::foundation::Result<AgentDebugInputSource> fromExportDraft(
        const AgentExportDraft& draft);

    static pathfinder::foundation::Result<AgentDebugInputSource> fromTestReport(
        const std::string& case_name,
        const AgentDebugReport& report);
};

} // namespace pathfinder::agent
