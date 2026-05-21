// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#include "pathfinder/h5_projection/projection.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace pathfinder::h5_projection {

namespace {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;

Result<void> fail(const std::string& message_key, const std::string& debug_message = {}) {
    return Result<void>::fail(pathfinder::foundation::makeError(
        ErrorCode::validation_failed,
        message_key,
        debug_message.empty() ? std::optional<std::string>{} : std::optional<std::string>{debug_message}));
}

template <typename T>
pathfinder::foundation::Result<T> failEnum(const std::string& message_key, const std::string& debug_message) {
    return pathfinder::foundation::Result<T>::fail(pathfinder::foundation::makeError(
        ErrorCode::validation_enum_unknown,
        message_key,
        debug_message));
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool containsAnyForbidden(const std::string& value) {
    const auto lowered = lower(value);
    static const std::vector<std::string> forbidden = {
        "hidden_truth", "true_trait", "real_effect", "raw_state", "gamestate",
        "actual_hp", "true_hp", "hp_delta", "death", "kill", "corpse",
        "loot", "drop", "random_damage", "frontend_unlock", "direct_state",
        "raw_replay_log", "raw_save_package", "internal_probability",
        "unmasked_condition", "secret_config", "<script", "javascript:",
        "onerror=", "onclick=", "<iframe"
    };
    for (const auto& key : forbidden) {
        if (lowered.find(key) != std::string::npos) return true;
    }
    return false;
}

Result<void> validateSafeKey(const std::string& value, const std::string& field, bool required = true) {
    if (required && value.empty()) return fail(field + ".missing");
    if (!value.empty() && containsProjectionForbiddenKey(value)) return fail(field + ".forbidden_key", value);
    return Result<void>::ok();
}

Result<void> validateSafeKeys(const std::vector<std::string>& values, const std::string& field) {
    for (const auto& value : values) {
        auto result = validateSafeKey(value, field + ".item", true);
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> validateBusinessEnum(bool invalid, const std::string& field) {
    if (invalid) return fail(field + ".invalid_enum");
    return Result<void>::ok();
}

template <typename T>
Result<void> validateVector(const std::vector<T>& values) {
    for (const auto& value : values) {
        auto result = value.validateBasic();
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

template <typename T>
pathfinder::foundation::Result<std::vector<T>> validateAndReturnVector(std::vector<T> values) {
    auto result = validateVector(values);
    if (result.is_error()) return pathfinder::foundation::Result<std::vector<T>>::fail(result.errors());
    return pathfinder::foundation::Result<std::vector<T>>::ok(std::move(values));
}

bool includesSection(const H5ProjectionRequest& request, ProjectionSectionKind section) {
    if (request.requested_sections.empty()) return true;
    return std::find(request.requested_sections.begin(), request.requested_sections.end(), section) != request.requested_sections.end();
}

void addIssue(std::vector<ProjectionIssue>& issues, ProjectionIssue issue) {
    if (issue.validateBasic().is_ok()) issues.push_back(std::move(issue));
}

SafeTextProjection fallbackText(const std::string& key, const std::string& text) {
    return makeSafeText(key, SafeTextKind::Warning, text);
}

std::vector<ProjectionIssue> makeAudienceIssuesForPlayerDebugRequest() {
    std::vector<ProjectionIssue> issues;
    auto issue = makeProjectionIssue(
        ProjectionIssueKind::AudienceNotAllowed,
        "warning",
        fallbackText("projection.player_debug_removed", "玩家界面不能显示调试区块"),
        false);
    issue.section = ProjectionSectionKind::DebugPanel;
    issue.field_path = "debug_keys";
    issues.push_back(std::move(issue));
    return issues;
}


template <typename T>
void truncateVector(std::vector<T>& values, size_t max_items, H5GameProjection& projection, ProjectionSectionKind section, const std::string& field_path) {
    if (values.size() <= max_items) return;
    values.resize(max_items);
    projection.header.truncated = true;
    auto issue = makeProjectionIssue(
        ProjectionIssueKind::Truncated,
        "warning",
        fallbackText("projection.truncated", "投影内容过多，已截断显示"),
        false);
    issue.section = section;
    issue.field_path = field_path;
    addIssue(projection.issues, std::move(issue));
}

std::string replayStatusText(pathfinder::replay::GoldenReplayStatus status) {
    switch (status) {
        case pathfinder::replay::GoldenReplayStatus::Passed: return "复盘通过";
        case pathfinder::replay::GoldenReplayStatus::Failed: return "复盘失败";
        case pathfinder::replay::GoldenReplayStatus::Unknown: return "复盘状态未知";
        case pathfinder::replay::GoldenReplayStatus::TestOnly: return "测试复盘状态";
    }
    return "复盘状态未知";
}

} // namespace

std::string toString(ProjectionAudience value) {
    switch (value) {
        case ProjectionAudience::Unknown: return "unknown";
        case ProjectionAudience::Player: return "player";
        case ProjectionAudience::DeveloperDebug: return "developer_debug";
        case ProjectionAudience::Training: return "training";
        case ProjectionAudience::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ProjectionMode value) {
    switch (value) {
        case ProjectionMode::Unknown: return "unknown";
        case ProjectionMode::Full: return "full";
        case ProjectionMode::Delta: return "delta";
        case ProjectionMode::DebugSnapshot: return "debug_snapshot";
        case ProjectionMode::TrainingObservation: return "training_observation";
        case ProjectionMode::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ProjectionSectionKind value) {
    switch (value) {
        case ProjectionSectionKind::Unknown: return "unknown";
        case ProjectionSectionKind::Scene: return "scene";
        case ProjectionSectionKind::ObjectCards: return "object_cards";
        case ProjectionSectionKind::ActionBar: return "action_bar";
        case ProjectionSectionKind::KnowledgePanel: return "knowledge_panel";
        case ProjectionSectionKind::CognitionPanel: return "cognition_panel";
        case ProjectionSectionKind::ConditionHints: return "condition_hints";
        case ProjectionSectionKind::DangerHints: return "danger_hints";
        case ProjectionSectionKind::TribePanel: return "tribe_panel";
        case ProjectionSectionKind::ReplayPanel: return "replay_panel";
        case ProjectionSectionKind::DebugPanel: return "debug_panel";
        case ProjectionSectionKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ProjectionItemKind value) {
    switch (value) {
        case ProjectionItemKind::Unknown: return "unknown";
        case ProjectionItemKind::ObjectCard: return "object_card";
        case ProjectionItemKind::Action: return "action";
        case ProjectionItemKind::KnowledgeLine: return "knowledge_line";
        case ProjectionItemKind::CognitionLine: return "cognition_line";
        case ProjectionItemKind::ConditionHint: return "condition_hint";
        case ProjectionItemKind::DangerHint: return "danger_hint";
        case ProjectionItemKind::TribeStatus: return "tribe_status";
        case ProjectionItemKind::ReplayStatus: return "replay_status";
        case ProjectionItemKind::Warning: return "warning";
        case ProjectionItemKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ActionAffordanceKind value) {
    switch (value) {
        case ActionAffordanceKind::Unknown: return "unknown";
        case ActionAffordanceKind::TryEat: return "try_eat";
        case ActionAffordanceKind::TryUse: return "try_use";
        case ActionAffordanceKind::Inspect: return "inspect";
        case ActionAffordanceKind::Teach: return "teach";
        case ActionAffordanceKind::Combine: return "combine";
        case ActionAffordanceKind::Avoid: return "avoid";
        case ActionAffordanceKind::Coordinate: return "coordinate";
        case ActionAffordanceKind::DebugReplay: return "debug_replay";
        case ActionAffordanceKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ProjectionBuildStatus value) {
    switch (value) {
        case ProjectionBuildStatus::Unknown: return "unknown";
        case ProjectionBuildStatus::Complete: return "complete";
        case ProjectionBuildStatus::Partial: return "partial";
        case ProjectionBuildStatus::Blocked: return "blocked";
        case ProjectionBuildStatus::Failed: return "failed";
        case ProjectionBuildStatus::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(SafeTextKind value) {
    switch (value) {
        case SafeTextKind::Unknown: return "unknown";
        case SafeTextKind::DisplayName: return "display_name";
        case SafeTextKind::Description: return "description";
        case SafeTextKind::Feedback: return "feedback";
        case SafeTextKind::Hint: return "hint";
        case SafeTextKind::Warning: return "warning";
        case SafeTextKind::DebugSummary: return "debug_summary";
        case SafeTextKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ProjectionIssueKind value) {
    switch (value) {
        case ProjectionIssueKind::Unknown: return "unknown";
        case ProjectionIssueKind::ForbiddenKeyRejected: return "forbidden_key_rejected";
        case ProjectionIssueKind::AudienceNotAllowed: return "audience_not_allowed";
        case ProjectionIssueKind::MissingSourceProjection: return "missing_source_projection";
        case ProjectionIssueKind::InvalidSourceData: return "invalid_source_data";
        case ProjectionIssueKind::Truncated: return "truncated";
        case ProjectionIssueKind::UnsafeTextRejected: return "unsafe_text_rejected";
        case ProjectionIssueKind::TestOnly: return "test_only";
    }
    return "unknown";
}

#define PARSE_ENUM(FuncName, EnumType, ...) \
pathfinder::foundation::Result<EnumType> FuncName(const std::string& value) { \
    const std::string normalized = lower(value); \
    __VA_ARGS__ \
    return failEnum<EnumType>("h5_projection.enum_invalid", "Unknown enum value: " + value); \
}

PARSE_ENUM(projectionAudienceFromString, ProjectionAudience,
    if (normalized == "player") return pathfinder::foundation::Result<ProjectionAudience>::ok(ProjectionAudience::Player);
    if (normalized == "developer_debug") return pathfinder::foundation::Result<ProjectionAudience>::ok(ProjectionAudience::DeveloperDebug);
    if (normalized == "training") return pathfinder::foundation::Result<ProjectionAudience>::ok(ProjectionAudience::Training);
    if (normalized == "test_only") return pathfinder::foundation::Result<ProjectionAudience>::ok(ProjectionAudience::TestOnly);
)

PARSE_ENUM(projectionModeFromString, ProjectionMode,
    if (normalized == "full") return pathfinder::foundation::Result<ProjectionMode>::ok(ProjectionMode::Full);
    if (normalized == "delta") return pathfinder::foundation::Result<ProjectionMode>::ok(ProjectionMode::Delta);
    if (normalized == "debug_snapshot") return pathfinder::foundation::Result<ProjectionMode>::ok(ProjectionMode::DebugSnapshot);
    if (normalized == "training_observation") return pathfinder::foundation::Result<ProjectionMode>::ok(ProjectionMode::TrainingObservation);
    if (normalized == "test_only") return pathfinder::foundation::Result<ProjectionMode>::ok(ProjectionMode::TestOnly);
)

PARSE_ENUM(projectionSectionKindFromString, ProjectionSectionKind,
    if (normalized == "scene") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::Scene);
    if (normalized == "object_cards") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::ObjectCards);
    if (normalized == "action_bar") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::ActionBar);
    if (normalized == "knowledge_panel") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::KnowledgePanel);
    if (normalized == "cognition_panel") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::CognitionPanel);
    if (normalized == "condition_hints") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::ConditionHints);
    if (normalized == "danger_hints") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::DangerHints);
    if (normalized == "tribe_panel") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::TribePanel);
    if (normalized == "replay_panel") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::ReplayPanel);
    if (normalized == "debug_panel") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::DebugPanel);
    if (normalized == "test_only") return pathfinder::foundation::Result<ProjectionSectionKind>::ok(ProjectionSectionKind::TestOnly);
)

PARSE_ENUM(projectionItemKindFromString, ProjectionItemKind,
    if (normalized == "object_card") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::ObjectCard);
    if (normalized == "action") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::Action);
    if (normalized == "knowledge_line") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::KnowledgeLine);
    if (normalized == "cognition_line") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::CognitionLine);
    if (normalized == "condition_hint") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::ConditionHint);
    if (normalized == "danger_hint") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::DangerHint);
    if (normalized == "tribe_status") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::TribeStatus);
    if (normalized == "replay_status") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::ReplayStatus);
    if (normalized == "warning") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::Warning);
    if (normalized == "test_only") return pathfinder::foundation::Result<ProjectionItemKind>::ok(ProjectionItemKind::TestOnly);
)

PARSE_ENUM(actionAffordanceKindFromString, ActionAffordanceKind,
    if (normalized == "try_eat") return pathfinder::foundation::Result<ActionAffordanceKind>::ok(ActionAffordanceKind::TryEat);
    if (normalized == "try_use") return pathfinder::foundation::Result<ActionAffordanceKind>::ok(ActionAffordanceKind::TryUse);
    if (normalized == "inspect") return pathfinder::foundation::Result<ActionAffordanceKind>::ok(ActionAffordanceKind::Inspect);
    if (normalized == "teach") return pathfinder::foundation::Result<ActionAffordanceKind>::ok(ActionAffordanceKind::Teach);
    if (normalized == "combine") return pathfinder::foundation::Result<ActionAffordanceKind>::ok(ActionAffordanceKind::Combine);
    if (normalized == "avoid") return pathfinder::foundation::Result<ActionAffordanceKind>::ok(ActionAffordanceKind::Avoid);
    if (normalized == "coordinate") return pathfinder::foundation::Result<ActionAffordanceKind>::ok(ActionAffordanceKind::Coordinate);
    if (normalized == "debug_replay") return pathfinder::foundation::Result<ActionAffordanceKind>::ok(ActionAffordanceKind::DebugReplay);
    if (normalized == "test_only") return pathfinder::foundation::Result<ActionAffordanceKind>::ok(ActionAffordanceKind::TestOnly);
)

PARSE_ENUM(projectionBuildStatusFromString, ProjectionBuildStatus,
    if (normalized == "complete") return pathfinder::foundation::Result<ProjectionBuildStatus>::ok(ProjectionBuildStatus::Complete);
    if (normalized == "partial") return pathfinder::foundation::Result<ProjectionBuildStatus>::ok(ProjectionBuildStatus::Partial);
    if (normalized == "blocked") return pathfinder::foundation::Result<ProjectionBuildStatus>::ok(ProjectionBuildStatus::Blocked);
    if (normalized == "failed") return pathfinder::foundation::Result<ProjectionBuildStatus>::ok(ProjectionBuildStatus::Failed);
    if (normalized == "test_only") return pathfinder::foundation::Result<ProjectionBuildStatus>::ok(ProjectionBuildStatus::TestOnly);
)

PARSE_ENUM(safeTextKindFromString, SafeTextKind,
    if (normalized == "display_name") return pathfinder::foundation::Result<SafeTextKind>::ok(SafeTextKind::DisplayName);
    if (normalized == "description") return pathfinder::foundation::Result<SafeTextKind>::ok(SafeTextKind::Description);
    if (normalized == "feedback") return pathfinder::foundation::Result<SafeTextKind>::ok(SafeTextKind::Feedback);
    if (normalized == "hint") return pathfinder::foundation::Result<SafeTextKind>::ok(SafeTextKind::Hint);
    if (normalized == "warning") return pathfinder::foundation::Result<SafeTextKind>::ok(SafeTextKind::Warning);
    if (normalized == "debug_summary") return pathfinder::foundation::Result<SafeTextKind>::ok(SafeTextKind::DebugSummary);
    if (normalized == "test_only") return pathfinder::foundation::Result<SafeTextKind>::ok(SafeTextKind::TestOnly);
)

PARSE_ENUM(projectionIssueKindFromString, ProjectionIssueKind,
    if (normalized == "forbidden_key_rejected") return pathfinder::foundation::Result<ProjectionIssueKind>::ok(ProjectionIssueKind::ForbiddenKeyRejected);
    if (normalized == "audience_not_allowed") return pathfinder::foundation::Result<ProjectionIssueKind>::ok(ProjectionIssueKind::AudienceNotAllowed);
    if (normalized == "missing_source_projection") return pathfinder::foundation::Result<ProjectionIssueKind>::ok(ProjectionIssueKind::MissingSourceProjection);
    if (normalized == "invalid_source_data") return pathfinder::foundation::Result<ProjectionIssueKind>::ok(ProjectionIssueKind::InvalidSourceData);
    if (normalized == "truncated") return pathfinder::foundation::Result<ProjectionIssueKind>::ok(ProjectionIssueKind::Truncated);
    if (normalized == "unsafe_text_rejected") return pathfinder::foundation::Result<ProjectionIssueKind>::ok(ProjectionIssueKind::UnsafeTextRejected);
    if (normalized == "test_only") return pathfinder::foundation::Result<ProjectionIssueKind>::ok(ProjectionIssueKind::TestOnly);
)

#undef PARSE_ENUM

bool containsProjectionForbiddenKey(const std::string& value) {
    return containsAnyForbidden(value);
}

bool containsProjectionForbiddenKey(const std::vector<std::string>& values) {
    for (const auto& value : values) if (containsProjectionForbiddenKey(value)) return true;
    return false;
}

Result<void> SafeTextProjection::validateBasic() const {
    if (kind == SafeTextKind::Unknown || kind == SafeTextKind::TestOnly) return fail("h5_projection.safe_text.kind_invalid");
    auto key_result = validateSafeKey(text_key, "h5_projection.safe_text.text_key"); if (key_result.is_error()) return key_result;
    if (zh_cn.empty()) return fail("h5_projection.safe_text.zh_cn_missing");
    if (containsProjectionForbiddenKey(zh_cn)) return fail("h5_projection.safe_text.zh_cn_forbidden");
    return validateSafeKeys(reason_keys, "h5_projection.safe_text.reason_keys");
}

Result<void> StatusBadgeProjection::validateBasic() const {
    auto key_result = validateSafeKey(badge_key, "h5_projection.badge.badge_key"); if (key_result.is_error()) return key_result;
    auto label_result = label.validateBasic(); if (label_result.is_error()) return label_result;
    auto tone_result = validateSafeKey(tone_key, "h5_projection.badge.tone_key"); if (tone_result.is_error()) return tone_result;
    if (priority < -1000 || priority > 1000) return fail("h5_projection.badge.priority_out_of_range");
    return Result<void>::ok();
}

Result<void> ConditionSummaryProjection::validateBasic() const {
    auto key_result = validateSafeKey(condition_key, "h5_projection.condition.condition_key"); if (key_result.is_error()) return key_result;
    auto summary_result = summary_text.validateBasic(); if (summary_result.is_error()) return summary_result;
    return validateSafeKeys(reason_keys, "h5_projection.condition.reason_keys");
}

Result<void> ActionProjection::validateBasic() const {
    auto key_result = validateSafeKey(action_key, "h5_projection.action.action_key"); if (key_result.is_error()) return key_result;
    auto enum_result = validateBusinessEnum(affordance == ActionAffordanceKind::Unknown || affordance == ActionAffordanceKind::TestOnly, "h5_projection.action.affordance"); if (enum_result.is_error()) return enum_result;
    auto label_result = label.validateBasic(); if (label_result.is_error()) return label_result;
    auto input_result = validateSafeKey(input_text, "h5_projection.action.input_text", false); if (input_result.is_error()) return input_result;
    auto preview_result = validateSafeKey(command_preview_key, "h5_projection.action.command_preview_key", false); if (preview_result.is_error()) return preview_result;
    if (disabled_reason.has_value()) { auto disabled_result = disabled_reason->validateBasic(); if (disabled_result.is_error()) return disabled_result; }
    return validateSafeKeys(target_object_refs, "h5_projection.action.target_object_refs");
}

Result<void> ObjectCardProjection::validateBasic() const {
    auto ref_result = validateSafeKey(object_ref_key, "h5_projection.object.object_ref_key"); if (ref_result.is_error()) return ref_result;
    auto display_result = display_name.validateBasic(); if (display_result.is_error()) return display_result;
    auto desc_result = description.validateBasic(); if (desc_result.is_error()) return desc_result;
    auto visibility_result = validateSafeKey(visibility_key, "h5_projection.object.visibility_key"); if (visibility_result.is_error()) return visibility_result;
    auto tag_result = validateSafeKeys(safe_tags, "h5_projection.object.safe_tags"); if (tag_result.is_error()) return tag_result;
    auto badge_result = validateVector(knowledge_badges); if (badge_result.is_error()) return badge_result;
    auto action_result = validateVector(actions); if (action_result.is_error()) return action_result;
    return validateSafeKeys(warning_keys, "h5_projection.object.warning_keys");
}

Result<void> KnowledgeLineProjection::validateBasic() const {
    auto id_result = validateSafeKey(knowledge_id, "h5_projection.knowledge.knowledge_id"); if (id_result.is_error()) return id_result;
    for (const auto* text : {&owner_label, &subject_label, &predicate_label, &effect_summary}) {
        auto result = text->validateBasic(); if (result.is_error()) return result;
    }
    auto status_result = validateSafeKey(status_key, "h5_projection.knowledge.status_key"); if (status_result.is_error()) return status_result;
    if (confidence_label.has_value()) { auto result = confidence_label->validateBasic(); if (result.is_error()) return result; }
    return validateSafeKeys(reason_keys, "h5_projection.knowledge.reason_keys");
}

Result<void> CognitionLineProjection::validateBasic() const {
    auto key_result = validateSafeKey(cognition_key, "h5_projection.cognition.cognition_key"); if (key_result.is_error()) return key_result;
    for (const auto* text : {&target_label, &aspect_label, &claim_summary}) {
        auto result = text->validateBasic(); if (result.is_error()) return result;
    }
    auto band_result = validateSafeKey(confidence_band, "h5_projection.cognition.confidence_band", false); if (band_result.is_error()) return band_result;
    if (source_hint.has_value()) { auto result = source_hint->validateBasic(); if (result.is_error()) return result; }
    return Result<void>::ok();
}

Result<void> DangerHintProjection::validateBasic() const {
    auto key_result = validateSafeKey(danger_key, "h5_projection.danger.danger_key"); if (key_result.is_error()) return key_result;
    auto severity_result = severity_label.validateBasic(); if (severity_result.is_error()) return severity_result;
    auto hint_result = hint_text.validateBasic(); if (hint_result.is_error()) return hint_result;
    auto counter_result = validateVector(countermeasure_labels); if (counter_result.is_error()) return counter_result;
    auto related_result = validateSafeKeys(related_object_refs, "h5_projection.danger.related_object_refs"); if (related_result.is_error()) return related_result;
    return validateSafeKeys(warning_keys, "h5_projection.danger.warning_keys");
}

Result<void> TribeStatusProjection::validateBasic() const {
    auto ref_result = validateSafeKey(tribe_ref_key, "h5_projection.tribe.tribe_ref_key"); if (ref_result.is_error()) return ref_result;
    for (const auto* text : {&morale_label, &trust_label, &split_risk_label, &coordination_label, &conflict_label}) {
        if (text->has_value()) { auto result = text->value().validateBasic(); if (result.is_error()) return result; }
    }
    return validateSafeKeys(reason_keys, "h5_projection.tribe.reason_keys");
}

Result<void> ReplayStatusProjection::validateBasic() const {
    auto key_result = validateSafeKey(replay_key, "h5_projection.replay.replay_key"); if (key_result.is_error()) return key_result;
    auto status_result = status_label.validateBasic(); if (status_result.is_error()) return status_result;
    auto issue_result = validateVector(issue_summaries); if (issue_result.is_error()) return issue_result;
    auto trace_result = validateVector(condition_trace_summary); if (trace_result.is_error()) return trace_result;
    if (baseline_label.has_value()) { auto result = baseline_label->validateBasic(); if (result.is_error()) return result; }
    return Result<void>::ok();
}

Result<void> ProjectionIssue::validateBasic() const {
    if (issue_kind == ProjectionIssueKind::Unknown || issue_kind == ProjectionIssueKind::TestOnly) return fail("h5_projection.issue.kind_invalid");
    auto severity_result = validateSafeKey(severity_key, "h5_projection.issue.severity_key"); if (severity_result.is_error()) return severity_result;
    if (section.has_value() && (*section == ProjectionSectionKind::Unknown || *section == ProjectionSectionKind::TestOnly)) return fail("h5_projection.issue.section_invalid");
    auto field_result = validateSafeKey(field_path, "h5_projection.issue.field_path", false); if (field_result.is_error()) return field_result;
    return safe_summary.validateBasic();
}

Result<void> H5ProjectionHeader::validateBasic() const {
    auto id_result = validateSafeKey(projection_id, "h5_projection.header.projection_id"); if (id_result.is_error()) return id_result;
    auto version_result = validateSafeKey(protocol_version, "h5_projection.header.protocol_version"); if (version_result.is_error()) return version_result;
    auto session_result = validateSafeKey(session_id, "h5_projection.header.session_id"); if (session_result.is_error()) return session_result;
    if (audience == ProjectionAudience::Unknown || audience == ProjectionAudience::TestOnly) return fail("h5_projection.header.audience_invalid");
    if (mode == ProjectionMode::Unknown || mode == ProjectionMode::TestOnly) return fail("h5_projection.header.mode_invalid");
    if (build_status == ProjectionBuildStatus::Unknown || build_status == ProjectionBuildStatus::TestOnly) return fail("h5_projection.header.build_status_invalid");
    if (config_version_id.has_value()) { auto result = validateSafeKey(config_version_id->value(), "h5_projection.header.config_version_id"); if (result.is_error()) return result; }
    return Result<void>::ok();
}

Result<void> H5GameProjection::validateBasic() const {
    auto header_result = header.validateBasic(); if (header_result.is_error()) return header_result;
    auto title_result = scene_title.validateBasic(); if (title_result.is_error()) return title_result;
    for (const auto& vector_result : {validateVector(scene_summary), validateVector(object_cards), validateVector(action_bar), validateVector(actor_knowledge), validateVector(recipient_knowledge), validateVector(cognition_lines), validateVector(condition_hints), validateVector(danger_hints), validateVector(issues)}) {
        if (vector_result.is_error()) return vector_result;
    }
    if (tribe_status.has_value()) { auto result = tribe_status->validateBasic(); if (result.is_error()) return result; }
    if (replay_status.has_value()) { auto result = replay_status->validateBasic(); if (result.is_error()) return result; }
    auto debug_result = validateSafeKeys(debug_keys, "h5_projection.debug_keys"); if (debug_result.is_error()) return debug_result;
    return validateSafeKeys(warning_keys, "h5_projection.warning_keys");
}

Result<void> H5ProjectionRequest::validateBasic() const {
    auto request_result = validateSafeKey(request_id, "h5_projection.request.request_id"); if (request_result.is_error()) return request_result;
    auto session_result = validateSafeKey(session_id, "h5_projection.request.session_id"); if (session_result.is_error()) return session_result;
    if (audience == ProjectionAudience::Unknown || audience == ProjectionAudience::TestOnly) return fail("h5_projection.request.audience_invalid");
    if (mode == ProjectionMode::Unknown || mode == ProjectionMode::TestOnly) return fail("h5_projection.request.mode_invalid");
    if (language != "zh_cn") return fail("h5_projection.request.language_unsupported");
    if (max_items_per_section == 0 || max_items_per_section > 512) return fail("h5_projection.request.max_items_invalid");
    if (audience == ProjectionAudience::Player && include_debug_trace) return fail("h5_projection.request.player_debug_forbidden");
    for (const auto& section : requested_sections) {
        if (section == ProjectionSectionKind::Unknown || section == ProjectionSectionKind::TestOnly) return fail("h5_projection.request.section_invalid");
        if (audience == ProjectionAudience::Player && section == ProjectionSectionKind::DebugPanel) return fail("h5_projection.request.player_debug_section_forbidden");
    }
    return Result<void>::ok();
}

Result<void> H5ProjectionSourceBundle::validateBasic() const {
    auto title_result = scene_title.validateBasic(); if (title_result.is_error()) return title_result;
    for (const auto& vector_result : {validateVector(scene_summary), validateVector(object_cards), validateVector(action_bar), validateVector(actor_knowledge), validateVector(recipient_knowledge), validateVector(cognition_lines), validateVector(condition_hints), validateVector(danger_hints)}) {
        if (vector_result.is_error()) return vector_result;
    }
    if (tribe_status.has_value()) { auto result = tribe_status->validateBasic(); if (result.is_error()) return result; }
    if (replay_status.has_value()) { auto result = replay_status->validateBasic(); if (result.is_error()) return result; }
    auto debug_result = validateSafeKeys(debug_keys, "h5_projection.source.debug_keys"); if (debug_result.is_error()) return debug_result;
    return validateSafeKeys(warning_keys, "h5_projection.source.warning_keys");
}

namespace {

Result<void> validateAudienceSafeText(const SafeTextProjection& text, ProjectionAudience audience) {
    if (audience == ProjectionAudience::Player && text.kind == SafeTextKind::DebugSummary) {
        return fail("h5_projection.player.debug_summary_forbidden");
    }
    return Result<void>::ok();
}

Result<void> validateAudienceTextList(const std::vector<SafeTextProjection>& texts, ProjectionAudience audience) {
    for (const auto& text : texts) {
        auto result = validateAudienceSafeText(text, audience);
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> validateAudienceCondition(const ConditionSummaryProjection& condition, ProjectionAudience audience) {
    return validateAudienceSafeText(condition.summary_text, audience);
}

Result<void> validateAudienceAction(const ActionProjection& action, ProjectionAudience audience) {
    auto label_result = validateAudienceSafeText(action.label, audience);
    if (label_result.is_error()) return label_result;
    if (action.disabled_reason.has_value()) return validateAudienceCondition(*action.disabled_reason, audience);
    return Result<void>::ok();
}

Result<void> validateAudienceActionList(const std::vector<ActionProjection>& actions, ProjectionAudience audience) {
    for (const auto& action : actions) {
        auto result = validateAudienceAction(action, audience);
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> validateAudienceObjectCard(const ObjectCardProjection& card, ProjectionAudience audience) {
    auto display_result = validateAudienceSafeText(card.display_name, audience);
    if (display_result.is_error()) return display_result;
    auto description_result = validateAudienceSafeText(card.description, audience);
    if (description_result.is_error()) return description_result;
    for (const auto& badge : card.knowledge_badges) {
        auto badge_result = validateAudienceSafeText(badge.label, audience);
        if (badge_result.is_error()) return badge_result;
    }
    return validateAudienceActionList(card.actions, audience);
}

Result<void> validateAudienceKnowledgeLine(const KnowledgeLineProjection& line, ProjectionAudience audience) {
    for (const auto* text : {&line.owner_label, &line.subject_label, &line.predicate_label, &line.effect_summary}) {
        auto result = validateAudienceSafeText(*text, audience);
        if (result.is_error()) return result;
    }
    if (line.confidence_label.has_value()) return validateAudienceSafeText(*line.confidence_label, audience);
    return Result<void>::ok();
}

Result<void> validateAudienceCognitionLine(const CognitionLineProjection& line, ProjectionAudience audience) {
    for (const auto* text : {&line.target_label, &line.aspect_label, &line.claim_summary}) {
        auto result = validateAudienceSafeText(*text, audience);
        if (result.is_error()) return result;
    }
    if (line.source_hint.has_value()) return validateAudienceSafeText(*line.source_hint, audience);
    return Result<void>::ok();
}

Result<void> validateAudienceDangerHint(const DangerHintProjection& hint, ProjectionAudience audience) {
    auto severity_result = validateAudienceSafeText(hint.severity_label, audience);
    if (severity_result.is_error()) return severity_result;
    auto hint_result = validateAudienceSafeText(hint.hint_text, audience);
    if (hint_result.is_error()) return hint_result;
    return validateAudienceTextList(hint.countermeasure_labels, audience);
}

Result<void> validateAudienceTribeStatus(const TribeStatusProjection& status, ProjectionAudience audience) {
    for (const auto* text : {&status.morale_label, &status.trust_label, &status.split_risk_label, &status.coordination_label, &status.conflict_label}) {
        if (text->has_value()) {
            auto result = validateAudienceSafeText(**text, audience);
            if (result.is_error()) return result;
        }
    }
    return Result<void>::ok();
}

Result<void> validateAudienceReplayStatus(const ReplayStatusProjection& status, ProjectionAudience audience) {
    auto status_result = validateAudienceSafeText(status.status_label, audience);
    if (status_result.is_error()) return status_result;
    auto issue_result = validateAudienceTextList(status.issue_summaries, audience);
    if (issue_result.is_error()) return issue_result;
    for (const auto& condition : status.condition_trace_summary) {
        auto result = validateAudienceCondition(condition, audience);
        if (result.is_error()) return result;
    }
    if (status.baseline_label.has_value()) return validateAudienceSafeText(*status.baseline_label, audience);
    return Result<void>::ok();
}

Result<void> validateProjectionAudienceTexts(const H5GameProjection& projection) {
    const auto audience = projection.header.audience;
    auto title_result = validateAudienceSafeText(projection.scene_title, audience);
    if (title_result.is_error()) return title_result;
    auto scene_result = validateAudienceTextList(projection.scene_summary, audience);
    if (scene_result.is_error()) return scene_result;
    for (const auto& card : projection.object_cards) {
        auto result = validateAudienceObjectCard(card, audience);
        if (result.is_error()) return result;
    }
    auto action_result = validateAudienceActionList(projection.action_bar, audience);
    if (action_result.is_error()) return action_result;
    for (const auto& line : projection.actor_knowledge) {
        auto result = validateAudienceKnowledgeLine(line, audience);
        if (result.is_error()) return result;
    }
    for (const auto& line : projection.recipient_knowledge) {
        auto result = validateAudienceKnowledgeLine(line, audience);
        if (result.is_error()) return result;
    }
    for (const auto& line : projection.cognition_lines) {
        auto result = validateAudienceCognitionLine(line, audience);
        if (result.is_error()) return result;
    }
    for (const auto& condition : projection.condition_hints) {
        auto result = validateAudienceCondition(condition, audience);
        if (result.is_error()) return result;
    }
    for (const auto& hint : projection.danger_hints) {
        auto result = validateAudienceDangerHint(hint, audience);
        if (result.is_error()) return result;
    }
    if (projection.tribe_status.has_value()) {
        auto result = validateAudienceTribeStatus(*projection.tribe_status, audience);
        if (result.is_error()) return result;
    }
    if (projection.replay_status.has_value()) {
        auto result = validateAudienceReplayStatus(*projection.replay_status, audience);
        if (result.is_error()) return result;
    }
    for (const auto& issue : projection.issues) {
        auto result = validateAudienceSafeText(issue.safe_summary, audience);
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

} // namespace

Result<void> ProjectionSanitizer::validateRequest(const H5ProjectionRequest& request) const {
    return request.validateBasic();
}

Result<void> ProjectionSanitizer::validateSafeText(const SafeTextProjection& text, ProjectionAudience audience) const {
    auto result = text.validateBasic();
    if (result.is_error()) return result;
    if (audience == ProjectionAudience::Player && text.kind == SafeTextKind::DebugSummary) return fail("h5_projection.safe_text.player_debug_summary_forbidden");
    return Result<void>::ok();
}

Result<void> ProjectionSanitizer::validateProjection(const H5GameProjection& projection) const {
    auto result = projection.validateBasic();
    if (result.is_error()) return result;
    if (projection.header.audience == ProjectionAudience::Player) {
        if (!projection.debug_keys.empty()) return fail("h5_projection.player.debug_keys_forbidden");
        if (projection.replay_status.has_value() && !projection.replay_status->visible_to_player) return fail("h5_projection.player.replay_status_forbidden");
    }
    return validateProjectionAudienceTexts(projection);
}

Result<H5GameProjection> ProjectionSanitizer::filterForAudience(H5GameProjection projection, ProjectionAudience audience) const {
    if (audience == ProjectionAudience::Player) {
        if (!projection.debug_keys.empty()) {
            for (auto& issue : makeAudienceIssuesForPlayerDebugRequest()) projection.issues.push_back(std::move(issue));
        }
        projection.debug_keys.clear();
        if (projection.replay_status.has_value() && !projection.replay_status->visible_to_player) {
            addIssue(projection.issues, makeProjectionIssue(
                ProjectionIssueKind::AudienceNotAllowed,
                "info",
                fallbackText("projection.replay_hidden_for_player", "复盘诊断仅在调试界面显示"),
                false));
            projection.replay_status.reset();
        }
    }
    auto result = validateProjection(projection);
    if (result.is_error()) return Result<H5GameProjection>::fail(result.errors());
    return Result<H5GameProjection>::ok(std::move(projection));
}

Result<std::vector<ObjectCardProjection>> ObjectCardProjectionBuilder::build(std::vector<ObjectCardProjection> cards) const {
    return validateAndReturnVector(std::move(cards));
}

Result<std::vector<ActionProjection>> ActionAffordanceProjectionBuilder::build(std::vector<ActionProjection> actions) const {
    return validateAndReturnVector(std::move(actions));
}

Result<std::vector<KnowledgeLineProjection>> KnowledgePanelProjectionBuilder::buildKnowledgeLines(std::vector<KnowledgeLineProjection> lines) const {
    return validateAndReturnVector(std::move(lines));
}

Result<std::vector<CognitionLineProjection>> KnowledgePanelProjectionBuilder::buildCognitionLines(std::vector<CognitionLineProjection> lines) const {
    return validateAndReturnVector(std::move(lines));
}

Result<std::vector<ConditionSummaryProjection>> ConditionSummaryProjectionBuilder::build(std::vector<ConditionSummaryProjection> conditions) const {
    return validateAndReturnVector(std::move(conditions));
}

Result<std::vector<DangerHintProjection>> DangerHintProjectionBuilder::build(std::vector<DangerHintProjection> hints) const {
    return validateAndReturnVector(std::move(hints));
}

Result<std::optional<TribeStatusProjection>> TribeStatusProjectionBuilder::build(std::optional<TribeStatusProjection> status) const {
    if (status.has_value()) { auto result = status->validateBasic(); if (result.is_error()) return Result<std::optional<TribeStatusProjection>>::fail(result.errors()); }
    return Result<std::optional<TribeStatusProjection>>::ok(std::move(status));
}

Result<std::optional<ReplayStatusProjection>> ReplayStatusProjectionBuilder::build(std::optional<ReplayStatusProjection> status, ProjectionAudience audience) const {
    if (audience == ProjectionAudience::Player && status.has_value() && !status->visible_to_player) return Result<std::optional<ReplayStatusProjection>>::ok(std::nullopt);
    if (status.has_value()) { auto result = status->validateBasic(); if (result.is_error()) return Result<std::optional<ReplayStatusProjection>>::fail(result.errors()); }
    return Result<std::optional<ReplayStatusProjection>>::ok(std::move(status));
}

Result<ReplayStatusProjection> ReplayStatusProjectionBuilder::fromGoldenReplayReport(const pathfinder::replay::GoldenReplayReport& report) const {
    auto report_result = report.validateBasic();
    if (report_result.is_error()) return Result<ReplayStatusProjection>::fail(report_result.errors());
    ReplayStatusProjection projection;
    projection.replay_key = "replay_status_" + toString(report.status);
    projection.status_label = makeSafeText("replay.status." + toString(report.status), SafeTextKind::DebugSummary, replayStatusText(report.status));
    projection.visible_to_player = false;
    for (const auto& issue : report.issues) {
        projection.issue_summaries.push_back(makeSafeText("replay.issue." + pathfinder::replay::toString(issue.kind), SafeTextKind::DebugSummary, issue.safe_summary_text));
    }
    for (const auto& key : report.observed_condition_trace_keys) {
        ConditionSummaryProjection condition;
        condition.condition_key = key;
        condition.summary_text = makeSafeText("replay.condition_trace", SafeTextKind::DebugSummary, "复盘条件轨迹：" + key);
        condition.blocking = false;
        projection.condition_trace_summary.push_back(std::move(condition));
    }
    projection.baseline_label = makeSafeText("replay.baseline.summary", SafeTextKind::DebugSummary, report.baseline.has_value() ? "已绑定黄金基线" : "未绑定黄金基线");
    auto result = projection.validateBasic();
    if (result.is_error()) return Result<ReplayStatusProjection>::fail(result.errors());
    return Result<ReplayStatusProjection>::ok(std::move(projection));
}

Result<H5GameProjection> H5ProjectionComposer::compose(const H5ProjectionRequest& request, const H5ProjectionSourceBundle& sources) const {
    auto request_result = sanitizer_.validateRequest(request);
    if (request_result.is_error()) return Result<H5GameProjection>::fail(request_result.errors());
    auto source_result = sources.validateBasic();
    if (source_result.is_error()) return Result<H5GameProjection>::fail(source_result.errors());

    H5GameProjection projection;
    projection.header.projection_id = "h5_projection_" + request.request_id;
    projection.header.protocol_version = "1.0";
    projection.header.session_id = request.session_id;
    projection.header.audience = request.audience;
    projection.header.mode = request.mode;
    projection.header.build_status = ProjectionBuildStatus::Complete;
    projection.scene_title = sources.scene_title;

    if (includesSection(request, ProjectionSectionKind::Scene)) projection.scene_summary = sources.scene_summary;
    if (includesSection(request, ProjectionSectionKind::ObjectCards)) projection.object_cards = sources.object_cards;
    if (includesSection(request, ProjectionSectionKind::ActionBar)) projection.action_bar = sources.action_bar;
    if (includesSection(request, ProjectionSectionKind::KnowledgePanel)) {
        projection.actor_knowledge = sources.actor_knowledge;
        projection.recipient_knowledge = sources.recipient_knowledge;
    }
    if (includesSection(request, ProjectionSectionKind::CognitionPanel)) projection.cognition_lines = sources.cognition_lines;
    if (includesSection(request, ProjectionSectionKind::ConditionHints)) projection.condition_hints = sources.condition_hints;
    if (includesSection(request, ProjectionSectionKind::DangerHints)) projection.danger_hints = sources.danger_hints;
    if (includesSection(request, ProjectionSectionKind::TribePanel)) projection.tribe_status = sources.tribe_status;
    if (includesSection(request, ProjectionSectionKind::ReplayPanel)) projection.replay_status = sources.replay_status;
    if (request.include_debug_trace && includesSection(request, ProjectionSectionKind::DebugPanel)) projection.debug_keys = sources.debug_keys;
    projection.warning_keys = sources.warning_keys;

    truncateVector(projection.scene_summary, request.max_items_per_section, projection, ProjectionSectionKind::Scene, "scene_summary");
    truncateVector(projection.object_cards, request.max_items_per_section, projection, ProjectionSectionKind::ObjectCards, "object_cards");
    truncateVector(projection.action_bar, request.max_items_per_section, projection, ProjectionSectionKind::ActionBar, "action_bar");
    truncateVector(projection.actor_knowledge, request.max_items_per_section, projection, ProjectionSectionKind::KnowledgePanel, "actor_knowledge");
    truncateVector(projection.recipient_knowledge, request.max_items_per_section, projection, ProjectionSectionKind::KnowledgePanel, "recipient_knowledge");
    truncateVector(projection.cognition_lines, request.max_items_per_section, projection, ProjectionSectionKind::CognitionPanel, "cognition_lines");
    truncateVector(projection.condition_hints, request.max_items_per_section, projection, ProjectionSectionKind::ConditionHints, "condition_hints");
    truncateVector(projection.danger_hints, request.max_items_per_section, projection, ProjectionSectionKind::DangerHints, "danger_hints");

    auto filtered = sanitizer_.filterForAudience(std::move(projection), request.audience);
    if (filtered.is_error()) return Result<H5GameProjection>::fail(filtered.errors());
    return filtered;
}

pathfinder::protocol::ProtocolEnvelope H5ProtocolProjectionAdapter::buildProjectionEnvelope(
    const H5GameProjection& projection,
    const H5ProjectionProtocolOptions& options) const {
    pathfinder::protocol::ProtocolEnvelope envelope;
    envelope.protocol_version = options.protocol_version;
    envelope.message_id = "h5_game_projection_" + projection.header.projection_id;
    envelope.message_type = pathfinder::protocol::ProtocolMessageType::Response;
    envelope.domain = pathfinder::protocol::ProtocolDomain::ProjectionSync;
    envelope.payload.payload_type = pathfinder::protocol::ProtocolPayloadType::H5GameProjection;
    envelope.payload.message_key = "h5.game_projection";
    envelope.request_id = options.request_id;
    envelope.correlation_id = options.correlation_id;
    envelope.session_id = options.session_id.has_value() ? options.session_id : std::optional<std::string>{projection.header.session_id};
    envelope.state_version = projection.header.state_version;
    envelope.config_version_id = projection.header.config_version_id;
    return envelope;
}

SafeTextProjection makeSafeText(std::string key, SafeTextKind kind, std::string zh_cn) {
    SafeTextProjection text;
    text.text_key = std::move(key);
    text.kind = kind;
    text.zh_cn = std::move(zh_cn);
    return text;
}

ProjectionIssue makeProjectionIssue(
    ProjectionIssueKind kind,
    std::string severity_key,
    SafeTextProjection safe_summary,
    bool blocked_output) {
    ProjectionIssue issue;
    issue.issue_kind = kind;
    issue.severity_key = std::move(severity_key);
    issue.safe_summary = std::move(safe_summary);
    issue.blocked_output = blocked_output;
    return issue;
}

} // namespace pathfinder::h5_projection
