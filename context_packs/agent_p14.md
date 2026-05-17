# Agent P14 Context Pack: Debug Input Adaptation Pre-Stage

## P14 Goal

P14 adds a safe Agent debug input adaptation layer after P13:

```text
P10: AgentHistoryProjection
P11: AgentDebugReport / AgentDiagnosticsSummary
P12: AgentExportDraft / AgentLocalExportService
P13: pathfinder_agent_debug_cli with built-in fixtures
P14: AgentDebugInputSource / Manifest / Validator / Adapter / Bundle
```

One-line goal:

```text
Allow CLI/debug tooling to consume validated safe debug input bundles without reading raw GameState, SaveGame, ObjectDefinition, AgentTickRecord, JSON, HTTP, or hidden truth.
```

## Phase Boundary

P14 is a tooling/input boundary phase.

P14 is NOT P15 cognition.
P14 is NOT memory.
P14 is NOT learning.
P14 is NOT save loading.
P14 is NOT frontend.

P14 must preserve the rule authority chain:

```text
CommandEnvelope -> RulePipeline -> StateChange/Event/Cognition
```

Debug input is read-only diagnostic material.

## Allowed Data Sources

P14 may consume only already-safe DTOs:

```text
P13 AgentDebugCliFixture / AgentDebugFixtureBundle
P10 AgentHistoryProjection
P11 AgentDebugReport
P11 AgentDiagnosticsSummary
P12 AgentExportDraft
In-memory test bundle
```

P14 must not consume:

```text
GameState
ObjectDefinition
AgentTickRecord raw records
SaveGame
SaveManager
JSON files
HTTP requests
WebSocket messages
H5 payloads
RL/LLM policy data
```

## New Files

```text
backend/include/pathfinder/agent/debug_input.h
backend/src/agent/debug_input.cpp
backend/tests/unit/agent/agent_debug_input_test.cpp
backend/tests/integration/p14/agent_debug_input_flow_test.cpp
backend/tests/integration/p14/agent_debug_input_security_test.cpp
context_packs/agent_p14.md
```

## Allowed Modifications

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
backend/tests/integration/p13/agent_debug_cli_flow_test.cpp
backend/tests/integration/p13/agent_debug_cli_security_test.cpp
```

P13 modifications are limited to routing fixture input through P14 or adding an internal `runWithInput` helper.
Do not change P13 CLI public behavior.

## Prohibited Modifications

```text
backend/src/rules/
backend/include/pathfinder/rules/
backend/src/object/
backend/include/pathfinder/object/
backend/src/pipeline/
backend/include/pathfinder/pipeline/
backend/src/replay/
backend/include/pathfinder/replay/
backend/src/protocol/
backend/include/pathfinder/protocol/
frontend/
SaveManager / HTTP / WebSocket related code
```

## Required Types

### ID

```cpp
struct AgentDebugInputIdTag {};
using AgentDebugInputId = pathfinder::foundation::StrongId<AgentDebugInputIdTag>;
```

Stable ID rules:

```text
fixture: agent_debug_input_fixture_<fixture_name>
projection: agent_debug_input_projection_<query_id>_<item_count>
report: agent_debug_input_report_<report_id>
export_draft: agent_debug_input_export_<manifest_id>
test: agent_debug_input_test_<case_name>
```

Do not use random IDs, timestamps, real paths, save paths, or private user data.

### Enums

```cpp
enum class AgentDebugInputKind {
    Unknown,
    BuiltinFixtureBundle,
    HistoryProjectionBundle,
    DebugReportBundle,
    ExportDraftBundle,
    InMemoryTestBundle
};

enum class AgentDebugInputTrustLevel {
    Unknown,
    Builtin,
    GeneratedSafeDto,
    TestOnly,
    ExternalUntrusted
};

enum class AgentDebugInputValidationStatus {
    Unknown,
    Valid,
    ValidWithWarnings,
    Invalid
};

enum class AgentDebugInputCapability {
    Unknown,
    CanBuildDebugReport,
    HasDiagnosticsSummary,
    HasExportDraft,
    CanWriteThroughP12,
    TestOnly
};

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
```

All enums need stable `toString/fromString` tests.

## Required DTOs

```cpp
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
```

## Required Services

```cpp
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
```

## Adaptation Rules

```text
BuiltinFixtureBundle -> call AgentDebugFixtureFactory -> report + diagnostics.
HistoryProjectionBundle -> call AgentDebugReportBuilder -> AgentDiagnosticsBuilder.
DebugReportBundle -> validate and pass report; build diagnostics if missing.
ExportDraftBundle -> allowed only with allow_export_draft_only=true; never reverse-build report from text.
InMemoryTestBundle -> allowed only with allow_test_only=true.
```

## Payload Exclusivity

```text
BuiltinFixtureBundle: fixture only.
HistoryProjectionBundle: projection only.
DebugReportBundle: report required, diagnostics optional.
ExportDraftBundle: export_draft only.
InMemoryTestBundle: report or projection allowed only when test_only=true.
```

Conflicting payloads must fail.

## Forbidden Keys

Centralize forbidden terms in:

```cpp
std::vector<std::string> agentDebugInputForbiddenKeys();
```

Required forbidden terms:

```text
edible_profile
hunger_delta
health_delta
effect_kind
ObjectDefinition
GameState
AgentTickRecord
reward_value
done =
is_done
SaveGame
SaveManager
```

Scan these places:

```text
manifest.source_label
manifest.warning_keys
report item summary_keys / reason_keys / phase_keys / warning_keys
diagnostics issue_key / detail_keys
export manifest warning_keys
export chunk title_key / payload
projection summary/status/warning style string fields
```

Normalize case before matching.
Return validation issues, do not assert.

## P13 Integration

Existing P13 public CLI behavior must remain valid.

Allowed internal shape:

```cpp
class AgentDebugCliRunner {
public:
    pathfinder::foundation::Result<AgentDebugCliResult> run(
        const AgentDebugCliOptions& options) const;

    pathfinder::foundation::Result<AgentDebugCliResult> runWithInput(
        const AgentDebugCliOptions& options,
        const AgentDebugInputSource& input) const;
};
```

Rules:

```text
runWithInput is internal C++ tooling/test API.
runWithInput must use AgentDebugInputAdapter.
Do not add --json / --save / --load-save / --http / --websocket.
Do not make CLI read files in P14.
```

## Required Tests

Unit tests:

```text
Enum toString/fromString for every P14 enum.
Factory stable IDs and capabilities.
Validator accepts fixture/report/projection.
Validator rejects Unknown kind/trust.
Validator rejects ExternalUntrusted.
Validator rejects missing/conflicting payload.
Validator rejects hidden truth and raw state keys.
Adapter supports fixture/report/projection.
Adapter rejects export_draft only by default.
Adapter accepts export_draft only when allow_export_draft_only=true and hasReport=false.
```

Integration tests:

```text
P13 unknown_fruit fixture through P14 adapter exports markdown.
P13 no_decision fixture through P14 adapter dry-run writes no files.
P11 report input through P14 adapter can reach P12 write planning.
P10 projection input through P14 adapter builds AgentDebugReport.
ExportDraftBundle never reverse-builds AgentDebugReport.
```

Security tests:

```text
source_label containing GameState fails.
summary_keys containing edible_profile fails.
reason_keys containing hunger_delta fails.
phase_keys containing health_delta fails.
warning_keys containing effect_kind fails.
export payload containing ObjectDefinition fails.
ExternalUntrusted fails.
test_only fails by default and succeeds with allow_test_only=true.
```

## Verification Commands

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_debug_input|p14" --output-on-failure
ctest --test-dir build/backend -R "agent|p14|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

Boundary scans:

```bash
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|SaveManager|httplib|asio|WebSocket|HTTP server|nlohmann|json|socket|send\(|recv\(" \
  backend/include/pathfinder/agent/debug_input.h \
  backend/src/agent/debug_input.cpp \
  backend/tests/integration/p14 \
  && exit 1 || true

rg -n "edible_profile|hunger_delta|health_delta|effect_kind|reward_value|done =|is_done|GameState|ObjectDefinition|AgentTickRecord" \
  backend/include/pathfinder/agent/debug_input.h \
  backend/src/agent/debug_input.cpp \
  backend/tests/integration/p14 \
  | rg -v "agentDebugInputForbiddenKeys|forbidden|RejectsHiddenTruth|RejectsRawState" \
  && exit 1 || true
```

## Completion Criteria

P14 is complete only when:

```text
debug_input.h/cpp exist.
All P14 DTOs validate themselves.
All P14 enums are round-trip tested.
Factory creates stable sources.
Validator rejects unsafe inputs.
Adapter creates standard safe bundles.
P13 fixture behavior remains compatible.
P10/P11/P12/P13 related regressions pass.
No JSON / SaveManager / HTTP / WebSocket / H5 / RL / LLM is added.
```

## Implementation Order

```text
1. Create debug_input.h with enums, DTOs, service declarations.
2. Implement enum string conversions and validateBasic basics.
3. Implement AgentDebugInputFactory for fixture/report.
4. Implement Validator manifest/payload/trust/schema checks.
5. Add forbidden key scanning.
6. Implement Adapter for fixture/report.
7. Add projection -> report adaptation.
8. Add export_draft only guarded path.
9. Optionally add P13 runWithInput or route fixture through adapter.
10. Add unit, flow, and security tests.
11. Run P14 targeted tests.
12. Run P10-P14 regression tests.
```
