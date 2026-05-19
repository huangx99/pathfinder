#pragma once

#include "pathfinder/foundation/hash.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/replay/replay_runner.h"
#include "pathfinder/save/save_package.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::replay {

struct GoldenReplayBaselineIdTag {};
using GoldenReplayBaselineId = pathfinder::foundation::StrongId<GoldenReplayBaselineIdTag>;

enum class GoldenReplayStatus {
    Unknown,
    Passed,
    Failed,
    TestOnly
};

enum class ReplayDiagnosticSeverity {
    Unknown,
    Info,
    Warning,
    Error,
    Critical,
    TestOnly
};

enum class ReplayDiagnosticIssueKind {
    Unknown,
    PackageIntegrityInvalid,
    RestorePlanInvalid,
    SnapshotMissing,
    ReplayExecutionFailed,
    ReplayMismatch,
    StateChangeCountMismatch,
    EventCountMismatch,
    PipelineStatusMismatch,
    StateVersionMismatch,
    ConditionTraceMissing,
    ConditionTraceUnexpected,
    BaselineMismatch,
    ForbiddenKeyDetected,
    TestOnly
};

std::string toString(GoldenReplayStatus status);
std::string toString(ReplayDiagnosticSeverity severity);
std::string toString(ReplayDiagnosticIssueKind kind);
pathfinder::foundation::Result<GoldenReplayStatus> goldenReplayStatusFromString(const std::string& value);
pathfinder::foundation::Result<ReplayDiagnosticSeverity> replayDiagnosticSeverityFromString(const std::string& value);
pathfinder::foundation::Result<ReplayDiagnosticIssueKind> replayDiagnosticIssueKindFromString(const std::string& value);

struct ReplayDiagnosticIssue {
    ReplayDiagnosticIssueKind kind{ReplayDiagnosticIssueKind::Unknown};
    ReplayDiagnosticSeverity severity{ReplayDiagnosticSeverity::Unknown};
    std::optional<pathfinder::foundation::CommandId> command_id;
    std::optional<pathfinder::save::SaveTraceId> trace_id;
    std::string field_path;
    std::string expected;
    std::string actual;
    std::string message_key;
    std::string safe_summary_text;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct GoldenReplayBaseline {
    GoldenReplayBaselineId baseline_id;
    pathfinder::save::SavePackageId source_save_id;
    pathfinder::foundation::HashValue package_hash;
    pathfinder::foundation::ConfigVersionId config_version;
    pathfinder::foundation::HashValue config_content_hash;
    pathfinder::foundation::StateVersion target_state_version;
    size_t expected_snapshot_count{0};
    size_t expected_command_count{0};
    size_t expected_random_count{0};
    size_t expected_event_count{0};
    std::vector<std::string> expected_condition_trace_keys;
    pathfinder::foundation::HashValue deterministic_signature;
    pathfinder::foundation::Tick created_tick;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct GoldenReplayRequest {
    pathfinder::save::SaveGamePackage package;
    pathfinder::foundation::StateVersion target_state_version;
    std::optional<GoldenReplayBaseline> baseline;
    std::vector<std::string> expected_condition_trace_keys;
    bool require_expected_condition_traces{true};
    bool allow_unexpected_condition_traces{true};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct GoldenReplayReport {
    GoldenReplayStatus status{GoldenReplayStatus::Unknown};
    pathfinder::foundation::HashValue package_hash;
    pathfinder::foundation::HashValue deterministic_signature;
    std::optional<GoldenReplayBaseline> baseline;
    std::optional<pathfinder::save::RestorePlan> restore_plan;
    ReplayCompareReport compare_report;
    std::vector<std::string> observed_condition_trace_keys;
    std::vector<std::string> missing_condition_trace_keys;
    std::vector<std::string> unexpected_condition_trace_keys;
    std::vector<ReplayDiagnosticIssue> issues;

    bool passed() const { return status == GoldenReplayStatus::Passed && issues.empty(); }
    pathfinder::foundation::Result<void> validateBasic() const;
};

class GoldenReplayBaselineBuilder {
public:
    pathfinder::foundation::Result<GoldenReplayBaseline> build(
        const pathfinder::save::SaveGamePackage& package,
        pathfinder::foundation::StateVersion target_state_version,
        std::vector<std::string> expected_condition_trace_keys = {}) const;
};

class GoldenReplayRunner {
public:
    pathfinder::foundation::Result<GoldenReplayReport> run(const GoldenReplayRequest& request) const;

private:
    mutable ReplayRunner replay_runner_;
};

pathfinder::foundation::HashValue calculateGoldenReplaySignature(
    const pathfinder::save::SaveGamePackage& package,
    pathfinder::foundation::StateVersion target_state_version,
    const std::vector<std::string>& condition_trace_keys);

std::vector<std::string> collectConditionTraceKeys(const pathfinder::save::SaveGamePackage& package);

} // namespace pathfinder::replay
