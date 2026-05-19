#include "pathfinder/replay/replay_diagnostics.h"
#include "pathfinder/condition/condition_expression_types.h"
#include <algorithm>
#include <sstream>
#include <set>

namespace pathfinder::replay {

namespace {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::HashValue;
using pathfinder::foundation::Result;

Result<void> failValidation(const std::string& message_key, const std::string& debug_message = {}) {
    auto error = pathfinder::foundation::makeError(
        ErrorCode::validation_failed,
        message_key,
        debug_message.empty() ? std::optional<std::string>{} : std::optional<std::string>{debug_message});
    return Result<void>::fail(std::move(error));
}

template <typename EnumValue>
Result<EnumValue> failEnum(const std::string& message_key) {
    return Result<EnumValue>::fail(pathfinder::foundation::makeError(
        ErrorCode::validation_enum_unknown,
        message_key));
}

bool hasForbiddenDiagnosticText(const std::string& value) {
    return pathfinder::save::containsSaveForbiddenKey(value)
        || pathfinder::condition::containsConditionForbiddenKey(value);
}

bool hasForbiddenDiagnosticText(const std::vector<std::string>& values) {
    for (const auto& value : values) {
        if (hasForbiddenDiagnosticText(value)) return true;
    }
    return false;
}

Result<void> validateIdText(const std::string& value, const std::string& message_key) {
    if (value.empty()) return failValidation(message_key + ".missing");
    if (!pathfinder::foundation::isValidIdString(value)) return failValidation(message_key + ".invalid_format");
    return Result<void>::ok();
}

Result<void> validateSafeText(const std::string& value, const std::string& message_key, bool required = false) {
    if (required && value.empty()) return failValidation(message_key + ".missing");
    if (hasForbiddenDiagnosticText(value)) return failValidation(message_key + ".forbidden_key");
    return Result<void>::ok();
}

Result<void> validateSafeTexts(const std::vector<std::string>& values, const std::string& message_key) {
    if (hasForbiddenDiagnosticText(values)) return failValidation(message_key + ".forbidden_key");
    for (const auto& value : values) {
        if (value.empty()) return failValidation(message_key + ".empty_value");
    }
    return Result<void>::ok();
}

std::vector<std::string> uniqueSorted(std::vector<std::string> values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

ReplayDiagnosticIssue makeIssue(
    ReplayDiagnosticIssueKind kind,
    ReplayDiagnosticSeverity severity,
    std::string field_path,
    std::string expected,
    std::string actual,
    std::string message_key,
    std::string safe_summary_text) {
    ReplayDiagnosticIssue issue;
    issue.kind = kind;
    issue.severity = severity;
    issue.field_path = std::move(field_path);
    issue.expected = std::move(expected);
    issue.actual = std::move(actual);
    issue.message_key = std::move(message_key);
    issue.safe_summary_text = std::move(safe_summary_text);
    return issue;
}

void addIssue(GoldenReplayReport& report, ReplayDiagnosticIssue issue) {
    auto validation = issue.validateBasic();
    if (validation.is_error()) {
        ReplayDiagnosticIssue forbidden;
        forbidden.kind = ReplayDiagnosticIssueKind::ForbiddenKeyDetected;
        forbidden.severity = ReplayDiagnosticSeverity::Critical;
        forbidden.field_path = "replay_diagnostic_issue";
        forbidden.expected = "safe_diagnostic_text";
        forbidden.actual = "forbidden_key_detected";
        forbidden.message_key = "p31.diagnostic_issue_forbidden_key";
        forbidden.safe_summary_text = "复盘诊断字段包含禁止暴露的隐藏信息";
        report.issues.push_back(std::move(forbidden));
        return;
    }
    report.issues.push_back(std::move(issue));
}

std::string commandCountString(const pathfinder::save::SaveGamePackage& package) {
    return std::to_string(package.command_log_section.command_log.size());
}

std::string randomCountString(const pathfinder::save::SaveGamePackage& package) {
    return std::to_string(package.random_log_section.random_log.size());
}

std::string eventCountString(const pathfinder::save::SaveGamePackage& package) {
    return std::to_string(package.event_log_section.events.size());
}

std::string snapshotCountString(const pathfinder::save::SaveGamePackage& package) {
    return std::to_string(package.snapshot_section.snapshots.size());
}

std::string hashString(HashValue hash) {
    return std::to_string(hash.value());
}

std::string stateVersionString(pathfinder::foundation::StateVersion state_version) {
    return std::to_string(state_version.value());
}

const pathfinder::save::StateSnapshotRecord* findSnapshot(
    const pathfinder::save::SaveGamePackage& package,
    const pathfinder::save::SaveSnapshotId& snapshot_id) {
    for (const auto& snapshot : package.snapshot_section.snapshots) {
        if (snapshot.snapshot_id == snapshot_id) return &snapshot;
    }
    return nullptr;
}

void addReplayCompareIssues(GoldenReplayReport& report) {
    if (report.compare_report.status != ReplayCompareStatus::Mismatch) return;

    addIssue(report, makeIssue(
        ReplayDiagnosticIssueKind::ReplayMismatch,
        ReplayDiagnosticSeverity::Error,
        "replay.compare.status",
        "match",
        "mismatch",
        "p31.replay_mismatch",
        "真实复盘结果与记录不一致"));

    if (report.compare_report.expected_state_change_count != report.compare_report.actual_state_change_count) {
        addIssue(report, makeIssue(
            ReplayDiagnosticIssueKind::StateChangeCountMismatch,
            ReplayDiagnosticSeverity::Error,
            "replay.compare.state_change_count",
            std::to_string(report.compare_report.expected_state_change_count),
            std::to_string(report.compare_report.actual_state_change_count),
            "p31.state_change_count_mismatch",
            "状态变更数量与记录不一致"));
    }

    if (report.compare_report.expected_event_count != report.compare_report.actual_event_count) {
        addIssue(report, makeIssue(
            ReplayDiagnosticIssueKind::EventCountMismatch,
            ReplayDiagnosticSeverity::Error,
            "replay.compare.event_count",
            std::to_string(report.compare_report.expected_event_count),
            std::to_string(report.compare_report.actual_event_count),
            "p31.event_count_mismatch",
            "事件数量与记录不一致"));
    }

    if (report.compare_report.expected_pipeline_status != report.compare_report.actual_pipeline_status) {
        addIssue(report, makeIssue(
            ReplayDiagnosticIssueKind::PipelineStatusMismatch,
            ReplayDiagnosticSeverity::Error,
            "replay.compare.pipeline_status",
            pathfinder::pipeline::toString(report.compare_report.expected_pipeline_status),
            pathfinder::pipeline::toString(report.compare_report.actual_pipeline_status),
            "p31.pipeline_status_mismatch",
            "规则管线状态与记录不一致"));
    }

    if (report.compare_report.expected_state_version_after != report.compare_report.actual_state_version_after) {
        addIssue(report, makeIssue(
            ReplayDiagnosticIssueKind::StateVersionMismatch,
            ReplayDiagnosticSeverity::Error,
            "replay.compare.state_version_after",
            stateVersionString(report.compare_report.expected_state_version_after),
            stateVersionString(report.compare_report.actual_state_version_after),
            "p31.state_version_mismatch",
            "复盘后的状态版本与记录不一致"));
    }
}

void addConditionTraceIssues(GoldenReplayReport& report, const GoldenReplayRequest& request) {
    std::vector<std::string> expected = request.expected_condition_trace_keys;
    if (expected.empty() && request.baseline.has_value()) {
        expected = request.baseline->expected_condition_trace_keys;
    }
    expected = uniqueSorted(std::move(expected));

    std::set<std::string> observed_set(report.observed_condition_trace_keys.begin(), report.observed_condition_trace_keys.end());
    std::set<std::string> expected_set(expected.begin(), expected.end());

    for (const auto& expected_key : expected) {
        if (!observed_set.contains(expected_key)) {
            report.missing_condition_trace_keys.push_back(expected_key);
            if (request.require_expected_condition_traces) {
                addIssue(report, makeIssue(
                    ReplayDiagnosticIssueKind::ConditionTraceMissing,
                    ReplayDiagnosticSeverity::Error,
                    "trace.condition_trace_keys",
                    expected_key,
                    "missing",
                    "p31.condition_trace_missing",
                    "期望的条件表达式 trace 没有出现"));
            }
        }
    }

    if (!request.allow_unexpected_condition_traces && !expected.empty()) {
        for (const auto& observed_key : report.observed_condition_trace_keys) {
            if (!expected_set.contains(observed_key)) {
                report.unexpected_condition_trace_keys.push_back(observed_key);
                addIssue(report, makeIssue(
                    ReplayDiagnosticIssueKind::ConditionTraceUnexpected,
                    ReplayDiagnosticSeverity::Warning,
                    "trace.condition_trace_keys",
                    "declared_condition_trace_only",
                    observed_key,
                    "p31.condition_trace_unexpected",
                    "复盘出现了未声明的条件表达式 trace"));
            }
        }
    }
}

void compareBaseline(GoldenReplayReport& report, const GoldenReplayRequest& request) {
    if (!request.baseline.has_value()) return;
    const auto& baseline = *request.baseline;
    auto addBaselineIssue = [&](std::string field_path, std::string expected, std::string actual, std::string summary) {
        addIssue(report, makeIssue(
            ReplayDiagnosticIssueKind::BaselineMismatch,
            ReplayDiagnosticSeverity::Error,
            std::move(field_path),
            std::move(expected),
            std::move(actual),
            "p31.baseline_mismatch",
            std::move(summary)));
    };

    if (baseline.source_save_id != request.package.header.save_id) {
        addBaselineIssue("baseline.source_save_id", baseline.source_save_id.value(), request.package.header.save_id.value(), "黄金基线来源存档不一致");
    }
    if (baseline.package_hash != request.package.header.package_hash) {
        addBaselineIssue("baseline.package_hash", hashString(baseline.package_hash), hashString(request.package.header.package_hash), "黄金基线 package hash 不一致");
    }
    if (baseline.config_version != request.package.config_manifest.active_config_version) {
        addBaselineIssue("baseline.config_version", baseline.config_version.value(), request.package.config_manifest.active_config_version.value(), "黄金基线配置版本不一致");
    }
    if (baseline.config_content_hash != request.package.config_manifest.config_content_hash) {
        addBaselineIssue("baseline.config_content_hash", hashString(baseline.config_content_hash), hashString(request.package.config_manifest.config_content_hash), "黄金基线配置内容 hash 不一致");
    }
    if (baseline.target_state_version != request.target_state_version) {
        addBaselineIssue("baseline.target_state_version", stateVersionString(baseline.target_state_version), stateVersionString(request.target_state_version), "黄金基线目标状态版本不一致");
    }
    if (baseline.expected_snapshot_count != request.package.snapshot_section.snapshots.size()) {
        addBaselineIssue("baseline.expected_snapshot_count", std::to_string(baseline.expected_snapshot_count), snapshotCountString(request.package), "黄金基线快照数量不一致");
    }
    if (baseline.expected_command_count != request.package.command_log_section.command_log.size()) {
        addBaselineIssue("baseline.expected_command_count", std::to_string(baseline.expected_command_count), commandCountString(request.package), "黄金基线命令数量不一致");
    }
    if (baseline.expected_random_count != request.package.random_log_section.random_log.size()) {
        addBaselineIssue("baseline.expected_random_count", std::to_string(baseline.expected_random_count), randomCountString(request.package), "黄金基线随机记录数量不一致");
    }
    if (baseline.expected_event_count != request.package.event_log_section.events.size()) {
        addBaselineIssue("baseline.expected_event_count", std::to_string(baseline.expected_event_count), eventCountString(request.package), "黄金基线事件数量不一致");
    }
    if (baseline.deterministic_signature != report.deterministic_signature) {
        addBaselineIssue("baseline.deterministic_signature", hashString(baseline.deterministic_signature), hashString(report.deterministic_signature), "黄金基线确定性签名不一致");
    }
}

} // namespace

std::string toString(GoldenReplayStatus status) {
    switch (status) {
        case GoldenReplayStatus::Unknown: return "unknown";
        case GoldenReplayStatus::Passed: return "passed";
        case GoldenReplayStatus::Failed: return "failed";
        case GoldenReplayStatus::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ReplayDiagnosticSeverity severity) {
    switch (severity) {
        case ReplayDiagnosticSeverity::Unknown: return "unknown";
        case ReplayDiagnosticSeverity::Info: return "info";
        case ReplayDiagnosticSeverity::Warning: return "warning";
        case ReplayDiagnosticSeverity::Error: return "error";
        case ReplayDiagnosticSeverity::Critical: return "critical";
        case ReplayDiagnosticSeverity::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ReplayDiagnosticIssueKind kind) {
    switch (kind) {
        case ReplayDiagnosticIssueKind::Unknown: return "unknown";
        case ReplayDiagnosticIssueKind::PackageIntegrityInvalid: return "package_integrity_invalid";
        case ReplayDiagnosticIssueKind::RestorePlanInvalid: return "restore_plan_invalid";
        case ReplayDiagnosticIssueKind::SnapshotMissing: return "snapshot_missing";
        case ReplayDiagnosticIssueKind::ReplayExecutionFailed: return "replay_execution_failed";
        case ReplayDiagnosticIssueKind::ReplayMismatch: return "replay_mismatch";
        case ReplayDiagnosticIssueKind::StateChangeCountMismatch: return "state_change_count_mismatch";
        case ReplayDiagnosticIssueKind::EventCountMismatch: return "event_count_mismatch";
        case ReplayDiagnosticIssueKind::PipelineStatusMismatch: return "pipeline_status_mismatch";
        case ReplayDiagnosticIssueKind::StateVersionMismatch: return "state_version_mismatch";
        case ReplayDiagnosticIssueKind::ConditionTraceMissing: return "condition_trace_missing";
        case ReplayDiagnosticIssueKind::ConditionTraceUnexpected: return "condition_trace_unexpected";
        case ReplayDiagnosticIssueKind::BaselineMismatch: return "baseline_mismatch";
        case ReplayDiagnosticIssueKind::ForbiddenKeyDetected: return "forbidden_key_detected";
        case ReplayDiagnosticIssueKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<GoldenReplayStatus> goldenReplayStatusFromString(const std::string& value) {
    if (value == "passed") return Result<GoldenReplayStatus>::ok(GoldenReplayStatus::Passed);
    if (value == "failed") return Result<GoldenReplayStatus>::ok(GoldenReplayStatus::Failed);
    if (value == "test_only") return Result<GoldenReplayStatus>::ok(GoldenReplayStatus::TestOnly);
    if (value == "unknown") return Result<GoldenReplayStatus>::ok(GoldenReplayStatus::Unknown);
    return failEnum<GoldenReplayStatus>("p31.status_unknown");
}

Result<ReplayDiagnosticSeverity> replayDiagnosticSeverityFromString(const std::string& value) {
    if (value == "info") return Result<ReplayDiagnosticSeverity>::ok(ReplayDiagnosticSeverity::Info);
    if (value == "warning") return Result<ReplayDiagnosticSeverity>::ok(ReplayDiagnosticSeverity::Warning);
    if (value == "error") return Result<ReplayDiagnosticSeverity>::ok(ReplayDiagnosticSeverity::Error);
    if (value == "critical") return Result<ReplayDiagnosticSeverity>::ok(ReplayDiagnosticSeverity::Critical);
    if (value == "test_only") return Result<ReplayDiagnosticSeverity>::ok(ReplayDiagnosticSeverity::TestOnly);
    if (value == "unknown") return Result<ReplayDiagnosticSeverity>::ok(ReplayDiagnosticSeverity::Unknown);
    return failEnum<ReplayDiagnosticSeverity>("p31.severity_unknown");
}

Result<ReplayDiagnosticIssueKind> replayDiagnosticIssueKindFromString(const std::string& value) {
    if (value == "package_integrity_invalid") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::PackageIntegrityInvalid);
    if (value == "restore_plan_invalid") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::RestorePlanInvalid);
    if (value == "snapshot_missing") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::SnapshotMissing);
    if (value == "replay_execution_failed") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::ReplayExecutionFailed);
    if (value == "replay_mismatch") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::ReplayMismatch);
    if (value == "state_change_count_mismatch") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::StateChangeCountMismatch);
    if (value == "event_count_mismatch") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::EventCountMismatch);
    if (value == "pipeline_status_mismatch") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::PipelineStatusMismatch);
    if (value == "state_version_mismatch") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::StateVersionMismatch);
    if (value == "condition_trace_missing") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::ConditionTraceMissing);
    if (value == "condition_trace_unexpected") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::ConditionTraceUnexpected);
    if (value == "baseline_mismatch") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::BaselineMismatch);
    if (value == "forbidden_key_detected") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::ForbiddenKeyDetected);
    if (value == "test_only") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::TestOnly);
    if (value == "unknown") return Result<ReplayDiagnosticIssueKind>::ok(ReplayDiagnosticIssueKind::Unknown);
    return failEnum<ReplayDiagnosticIssueKind>("p31.issue_kind_unknown");
}

Result<void> ReplayDiagnosticIssue::validateBasic() const {
    if (kind == ReplayDiagnosticIssueKind::Unknown || kind == ReplayDiagnosticIssueKind::TestOnly) return failValidation("p31.issue.kind_invalid");
    if (severity == ReplayDiagnosticSeverity::Unknown || severity == ReplayDiagnosticSeverity::TestOnly) return failValidation("p31.issue.severity_invalid");
    if (command_id.has_value()) { auto result = validateIdText(command_id->value(), "p31.issue.command_id"); if (result.is_error()) return result; }
    if (trace_id.has_value()) { auto result = validateIdText(trace_id->value(), "p31.issue.trace_id"); if (result.is_error()) return result; }
    auto field_result = validateSafeText(field_path, "p31.issue.field_path", true); if (field_result.is_error()) return field_result;
    auto expected_result = validateSafeText(expected, "p31.issue.expected"); if (expected_result.is_error()) return expected_result;
    auto actual_result = validateSafeText(actual, "p31.issue.actual"); if (actual_result.is_error()) return actual_result;
    auto message_result = validateSafeText(message_key, "p31.issue.message_key", true); if (message_result.is_error()) return message_result;
    auto summary_result = validateSafeText(safe_summary_text, "p31.issue.safe_summary_text", true); if (summary_result.is_error()) return summary_result;
    return Result<void>::ok();
}

Result<void> GoldenReplayBaseline::validateBasic() const {
    auto baseline_result = validateIdText(baseline_id.value(), "p31.baseline.baseline_id"); if (baseline_result.is_error()) return baseline_result;
    auto save_result = validateIdText(source_save_id.value(), "p31.baseline.source_save_id"); if (save_result.is_error()) return save_result;
    if (package_hash.empty()) return failValidation("p31.baseline.package_hash_missing");
    auto config_result = validateIdText(config_version.value(), "p31.baseline.config_version"); if (config_result.is_error()) return config_result;
    if (config_content_hash.empty()) return failValidation("p31.baseline.config_content_hash_missing");
    if (deterministic_signature.empty()) return failValidation("p31.baseline.signature_missing");
    return validateSafeTexts(expected_condition_trace_keys, "p31.baseline.expected_condition_trace_keys");
}

Result<void> GoldenReplayRequest::validateBasic() const {
    auto package_result = package.validateBasic(); if (package_result.is_error()) return package_result;
    auto trace_result = validateSafeTexts(expected_condition_trace_keys, "p31.request.expected_condition_trace_keys"); if (trace_result.is_error()) return trace_result;
    if (baseline.has_value()) {
        auto baseline_result = baseline->validateBasic(); if (baseline_result.is_error()) return baseline_result;
    }
    return Result<void>::ok();
}

Result<void> GoldenReplayReport::validateBasic() const {
    if (status == GoldenReplayStatus::Unknown || status == GoldenReplayStatus::TestOnly) return failValidation("p31.report.status_invalid");
    if (package_hash.empty()) return failValidation("p31.report.package_hash_missing");
    if (deterministic_signature.empty()) return failValidation("p31.report.signature_missing");
    auto observed_result = validateSafeTexts(observed_condition_trace_keys, "p31.report.observed_condition_trace_keys"); if (observed_result.is_error()) return observed_result;
    auto missing_result = validateSafeTexts(missing_condition_trace_keys, "p31.report.missing_condition_trace_keys"); if (missing_result.is_error()) return missing_result;
    auto unexpected_result = validateSafeTexts(unexpected_condition_trace_keys, "p31.report.unexpected_condition_trace_keys"); if (unexpected_result.is_error()) return unexpected_result;
    for (const auto& issue : issues) { auto result = issue.validateBasic(); if (result.is_error()) return result; }
    if (baseline.has_value()) { auto result = baseline->validateBasic(); if (result.is_error()) return result; }
    if (restore_plan.has_value()) { auto result = restore_plan->validateBasic(); if (result.is_error()) return result; }
    return Result<void>::ok();
}

std::vector<std::string> collectConditionTraceKeys(const pathfinder::save::SaveGamePackage& package) {
    std::vector<std::string> keys;
    for (const auto& trace : package.trace_section.traces) {
        for (const auto& key : trace.condition_trace_keys) keys.push_back(key);
    }
    return uniqueSorted(std::move(keys));
}

HashValue calculateGoldenReplaySignature(
    const pathfinder::save::SaveGamePackage& package,
    pathfinder::foundation::StateVersion target_state_version,
    const std::vector<std::string>& condition_trace_keys) {
    auto keys = uniqueSorted(condition_trace_keys);
    std::ostringstream stream;
    stream << "golden_replay.v1";
    stream << "|save=" << package.header.save_id.value();
    stream << "|package=" << package.header.package_hash.value();
    stream << "|config=" << package.config_manifest.active_config_version.value();
    stream << "|config_hash=" << package.config_manifest.config_content_hash.value();
    stream << "|target=" << target_state_version.value();
    stream << "|snapshots=" << package.snapshot_section.snapshots.size();
    stream << "|commands=" << package.command_log_section.command_log.size();
    stream << "|random=" << package.random_log_section.random_log.size();
    stream << "|events=" << package.event_log_section.events.size();
    for (const auto& key : keys) stream << "|trace=" << key;
    return HashValue::fromString(stream.str());
}

Result<GoldenReplayBaseline> GoldenReplayBaselineBuilder::build(
    const pathfinder::save::SaveGamePackage& package,
    pathfinder::foundation::StateVersion target_state_version,
    std::vector<std::string> expected_condition_trace_keys) const {
    auto package_result = pathfinder::save::SavePackageValidator{}.validate(package);
    if (package_result.is_error()) return Result<GoldenReplayBaseline>::fail(package_result.errors());

    auto plan_result = pathfinder::save::SaveRestorePlanner{}.plan(package, target_state_version);
    if (plan_result.is_error()) return Result<GoldenReplayBaseline>::fail(plan_result.errors());

    if (expected_condition_trace_keys.empty()) expected_condition_trace_keys = collectConditionTraceKeys(package);
    expected_condition_trace_keys = uniqueSorted(std::move(expected_condition_trace_keys));
    auto trace_result = validateSafeTexts(expected_condition_trace_keys, "p31.baseline_builder.expected_condition_trace_keys");
    if (trace_result.is_error()) return Result<GoldenReplayBaseline>::fail(trace_result.errors());

    GoldenReplayBaseline baseline;
    baseline.baseline_id = GoldenReplayBaselineId("golden_" + package.header.save_id.value());
    baseline.source_save_id = package.header.save_id;
    baseline.package_hash = package.header.package_hash;
    baseline.config_version = package.config_manifest.active_config_version;
    baseline.config_content_hash = package.config_manifest.config_content_hash;
    baseline.target_state_version = target_state_version;
    baseline.expected_snapshot_count = package.snapshot_section.snapshots.size();
    baseline.expected_command_count = package.command_log_section.command_log.size();
    baseline.expected_random_count = package.random_log_section.random_log.size();
    baseline.expected_event_count = package.event_log_section.events.size();
    baseline.expected_condition_trace_keys = std::move(expected_condition_trace_keys);
    baseline.deterministic_signature = calculateGoldenReplaySignature(package, target_state_version, baseline.expected_condition_trace_keys);
    baseline.created_tick = package.header.created_tick;

    auto baseline_result = baseline.validateBasic();
    if (baseline_result.is_error()) return Result<GoldenReplayBaseline>::fail(baseline_result.errors());
    return Result<GoldenReplayBaseline>::ok(std::move(baseline));
}

Result<GoldenReplayReport> GoldenReplayRunner::run(const GoldenReplayRequest& request) const {
    GoldenReplayReport report;
    report.status = GoldenReplayStatus::Failed;
    report.package_hash = request.package.header.package_hash;
    report.baseline = request.baseline;
    report.observed_condition_trace_keys = collectConditionTraceKeys(request.package);
    report.deterministic_signature = calculateGoldenReplaySignature(request.package, request.target_state_version, report.observed_condition_trace_keys);

    auto request_result = request.validateBasic();
    if (request_result.is_error()) return Result<GoldenReplayReport>::fail(request_result.errors());

    auto package_result = pathfinder::save::SavePackageValidator{}.validate(request.package);
    if (package_result.is_error()) {
        addIssue(report, makeIssue(
            ReplayDiagnosticIssueKind::PackageIntegrityInvalid,
            ReplayDiagnosticSeverity::Critical,
            "save_package.integrity",
            "valid_save_package",
            "invalid_save_package",
            "p31.package_integrity_invalid",
            "存档包完整性校验失败，不能执行真实复盘"));
        auto report_result = report.validateBasic();
        if (report_result.is_error()) return Result<GoldenReplayReport>::fail(report_result.errors());
        return Result<GoldenReplayReport>::ok(std::move(report));
    }

    auto plan_result = pathfinder::save::SaveRestorePlanner{}.plan(request.package, request.target_state_version);
    if (plan_result.is_error()) {
        addIssue(report, makeIssue(
            ReplayDiagnosticIssueKind::RestorePlanInvalid,
            ReplayDiagnosticSeverity::Error,
            "restore_plan.target_state_version",
            "target_inside_package_range",
            stateVersionString(request.target_state_version),
            "p31.restore_plan_invalid",
            "目标状态版本无法生成恢复计划"));
        auto report_result = report.validateBasic();
        if (report_result.is_error()) return Result<GoldenReplayReport>::fail(report_result.errors());
        return Result<GoldenReplayReport>::ok(std::move(report));
    }
    report.restore_plan = plan_result.value();

    const auto* snapshot = findSnapshot(request.package, plan_result.value().selected_snapshot_id);
    if (!snapshot) {
        addIssue(report, makeIssue(
            ReplayDiagnosticIssueKind::SnapshotMissing,
            ReplayDiagnosticSeverity::Critical,
            "restore_plan.selected_snapshot_id",
            plan_result.value().selected_snapshot_id.value(),
            "missing",
            "p31.snapshot_missing",
            "恢复计划选中的状态快照不存在"));
        auto report_result = report.validateBasic();
        if (report_result.is_error()) return Result<GoldenReplayReport>::fail(report_result.errors());
        return Result<GoldenReplayReport>::ok(std::move(report));
    }

    ReplayInput replay_input;
    replay_input.initial_state = snapshot->snapshot;
    replay_input.command_log = request.package.command_log_section.command_log;
    replay_input.random_log = request.package.random_log_section.random_log;

    auto replay_result = replay_runner_.replayOne(replay_input);
    if (replay_result.is_error()) {
        addIssue(report, makeIssue(
            ReplayDiagnosticIssueKind::ReplayExecutionFailed,
            ReplayDiagnosticSeverity::Error,
            "replay.execution",
            "replay_success",
            "replay_error",
            "p31.replay_execution_failed",
            "真实复盘执行失败"));
        auto report_result = report.validateBasic();
        if (report_result.is_error()) return Result<GoldenReplayReport>::fail(report_result.errors());
        return Result<GoldenReplayReport>::ok(std::move(report));
    }

    report.compare_report = replay_result.value().report;
    addReplayCompareIssues(report);
    addConditionTraceIssues(report, request);
    compareBaseline(report, request);

    report.status = report.issues.empty() ? GoldenReplayStatus::Passed : GoldenReplayStatus::Failed;
    auto report_result = report.validateBasic();
    if (report_result.is_error()) return Result<GoldenReplayReport>::fail(report_result.errors());
    return Result<GoldenReplayReport>::ok(std::move(report));
}

} // namespace pathfinder::replay
