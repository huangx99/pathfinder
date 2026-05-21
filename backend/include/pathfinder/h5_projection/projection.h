// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/protocol/envelope.h"
#include "pathfinder/replay/replay_diagnostics.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::h5_projection {

enum class ProjectionAudience {
    Unknown,
    Player,
    DeveloperDebug,
    Training,
    TestOnly
};

enum class ProjectionMode {
    Unknown,
    Full,
    Delta,
    DebugSnapshot,
    TrainingObservation,
    TestOnly
};

enum class ProjectionSectionKind {
    Unknown,
    Scene,
    ObjectCards,
    ActionBar,
    KnowledgePanel,
    CognitionPanel,
    ConditionHints,
    DangerHints,
    TribePanel,
    ReplayPanel,
    DebugPanel,
    TestOnly
};

enum class ProjectionItemKind {
    Unknown,
    ObjectCard,
    Action,
    KnowledgeLine,
    CognitionLine,
    ConditionHint,
    DangerHint,
    TribeStatus,
    ReplayStatus,
    Warning,
    TestOnly
};

enum class ActionAffordanceKind {
    Unknown,
    TryEat,
    TryUse,
    Inspect,
    Teach,
    Combine,
    Avoid,
    Coordinate,
    DebugReplay,
    TestOnly
};

enum class ProjectionBuildStatus {
    Unknown,
    Complete,
    Partial,
    Blocked,
    Failed,
    TestOnly
};

enum class SafeTextKind {
    Unknown,
    DisplayName,
    Description,
    Feedback,
    Hint,
    Warning,
    DebugSummary,
    TestOnly
};

enum class ProjectionIssueKind {
    Unknown,
    ForbiddenKeyRejected,
    AudienceNotAllowed,
    MissingSourceProjection,
    InvalidSourceData,
    Truncated,
    UnsafeTextRejected,
    TestOnly
};

std::string toString(ProjectionAudience value);
std::string toString(ProjectionMode value);
std::string toString(ProjectionSectionKind value);
std::string toString(ProjectionItemKind value);
std::string toString(ActionAffordanceKind value);
std::string toString(ProjectionBuildStatus value);
std::string toString(SafeTextKind value);
std::string toString(ProjectionIssueKind value);

pathfinder::foundation::Result<ProjectionAudience> projectionAudienceFromString(const std::string& value);
pathfinder::foundation::Result<ProjectionMode> projectionModeFromString(const std::string& value);
pathfinder::foundation::Result<ProjectionSectionKind> projectionSectionKindFromString(const std::string& value);
pathfinder::foundation::Result<ProjectionItemKind> projectionItemKindFromString(const std::string& value);
pathfinder::foundation::Result<ActionAffordanceKind> actionAffordanceKindFromString(const std::string& value);
pathfinder::foundation::Result<ProjectionBuildStatus> projectionBuildStatusFromString(const std::string& value);
pathfinder::foundation::Result<SafeTextKind> safeTextKindFromString(const std::string& value);
pathfinder::foundation::Result<ProjectionIssueKind> projectionIssueKindFromString(const std::string& value);

bool containsProjectionForbiddenKey(const std::string& value);
bool containsProjectionForbiddenKey(const std::vector<std::string>& values);

struct SafeTextProjection {
    std::string text_key;
    SafeTextKind kind{SafeTextKind::Unknown};
    std::string zh_cn;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StatusBadgeProjection {
    std::string badge_key;
    SafeTextProjection label;
    std::string tone_key;
    int priority{0};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConditionSummaryProjection {
    std::string condition_key;
    SafeTextProjection summary_text;
    std::optional<bool> satisfied_hint;
    bool blocking{false};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ActionProjection {
    std::string action_key;
    ActionAffordanceKind affordance{ActionAffordanceKind::Unknown};
    SafeTextProjection label;
    std::string input_text;
    bool enabled{true};
    std::optional<ConditionSummaryProjection> disabled_reason;
    std::string command_preview_key;
    std::vector<std::string> target_object_refs;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ObjectCardProjection {
    std::string object_ref_key;
    SafeTextProjection display_name;
    SafeTextProjection description;
    std::string visibility_key;
    std::vector<std::string> safe_tags;
    std::vector<StatusBadgeProjection> knowledge_badges;
    std::vector<ActionProjection> actions;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeLineProjection {
    std::string knowledge_id;
    SafeTextProjection owner_label;
    SafeTextProjection subject_label;
    SafeTextProjection predicate_label;
    SafeTextProjection effect_summary;
    std::string status_key;
    std::optional<SafeTextProjection> confidence_label;
    bool teachable{false};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionLineProjection {
    std::string cognition_key;
    SafeTextProjection target_label;
    SafeTextProjection aspect_label;
    SafeTextProjection claim_summary;
    std::string confidence_band;
    std::optional<SafeTextProjection> source_hint;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DangerHintProjection {
    std::string danger_key;
    SafeTextProjection severity_label;
    SafeTextProjection hint_text;
    std::vector<SafeTextProjection> countermeasure_labels;
    std::vector<std::string> related_object_refs;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeStatusProjection {
    std::string tribe_ref_key;
    std::optional<SafeTextProjection> morale_label;
    std::optional<SafeTextProjection> trust_label;
    std::optional<SafeTextProjection> split_risk_label;
    std::optional<SafeTextProjection> coordination_label;
    std::optional<SafeTextProjection> conflict_label;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReplayStatusProjection {
    std::string replay_key;
    SafeTextProjection status_label;
    std::vector<SafeTextProjection> issue_summaries;
    std::vector<ConditionSummaryProjection> condition_trace_summary;
    std::optional<SafeTextProjection> baseline_label;
    bool visible_to_player{false};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ProjectionIssue {
    ProjectionIssueKind issue_kind{ProjectionIssueKind::Unknown};
    std::string severity_key;
    std::optional<ProjectionSectionKind> section;
    std::string field_path;
    SafeTextProjection safe_summary;
    bool blocked_output{false};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct H5ProjectionHeader {
    std::string projection_id;
    std::string protocol_version{"1.0"};
    std::string session_id;
    ProjectionAudience audience{ProjectionAudience::Unknown};
    ProjectionMode mode{ProjectionMode::Unknown};
    std::optional<pathfinder::foundation::StateVersion> state_version;
    std::optional<pathfinder::foundation::ConfigVersionId> config_version_id;
    ProjectionBuildStatus build_status{ProjectionBuildStatus::Unknown};
    bool truncated{false};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct H5GameProjection {
    H5ProjectionHeader header;
    SafeTextProjection scene_title;
    std::vector<SafeTextProjection> scene_summary;
    std::vector<ObjectCardProjection> object_cards;
    std::vector<ActionProjection> action_bar;
    std::vector<KnowledgeLineProjection> actor_knowledge;
    std::vector<KnowledgeLineProjection> recipient_knowledge;
    std::vector<CognitionLineProjection> cognition_lines;
    std::vector<ConditionSummaryProjection> condition_hints;
    std::vector<DangerHintProjection> danger_hints;
    std::optional<TribeStatusProjection> tribe_status;
    std::optional<ReplayStatusProjection> replay_status;
    std::vector<ProjectionIssue> issues;
    std::vector<std::string> debug_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct H5ProjectionRequest {
    std::string request_id;
    std::string session_id;
    ProjectionAudience audience{ProjectionAudience::Unknown};
    ProjectionMode mode{ProjectionMode::Unknown};
    std::vector<ProjectionSectionKind> requested_sections;
    size_t max_items_per_section{64};
    bool include_debug_trace{false};
    std::string language{"zh_cn"};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct H5ProjectionSourceBundle {
    SafeTextProjection scene_title;
    std::vector<SafeTextProjection> scene_summary;
    std::vector<ObjectCardProjection> object_cards;
    std::vector<ActionProjection> action_bar;
    std::vector<KnowledgeLineProjection> actor_knowledge;
    std::vector<KnowledgeLineProjection> recipient_knowledge;
    std::vector<CognitionLineProjection> cognition_lines;
    std::vector<ConditionSummaryProjection> condition_hints;
    std::vector<DangerHintProjection> danger_hints;
    std::optional<TribeStatusProjection> tribe_status;
    std::optional<ReplayStatusProjection> replay_status;
    std::vector<std::string> debug_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct H5ProjectionProtocolOptions {
    std::string protocol_version{"1.0"};
    std::optional<std::string> request_id;
    std::optional<std::string> correlation_id;
    std::optional<std::string> session_id;
};

class ProjectionSanitizer {
public:
    pathfinder::foundation::Result<void> validateRequest(const H5ProjectionRequest& request) const;
    pathfinder::foundation::Result<void> validateSafeText(const SafeTextProjection& text, ProjectionAudience audience) const;
    pathfinder::foundation::Result<void> validateProjection(const H5GameProjection& projection) const;
    pathfinder::foundation::Result<H5GameProjection> filterForAudience(H5GameProjection projection, ProjectionAudience audience) const;
};

class ObjectCardProjectionBuilder {
public:
    pathfinder::foundation::Result<std::vector<ObjectCardProjection>> build(std::vector<ObjectCardProjection> cards) const;
};

class ActionAffordanceProjectionBuilder {
public:
    pathfinder::foundation::Result<std::vector<ActionProjection>> build(std::vector<ActionProjection> actions) const;
};

class KnowledgePanelProjectionBuilder {
public:
    pathfinder::foundation::Result<std::vector<KnowledgeLineProjection>> buildKnowledgeLines(std::vector<KnowledgeLineProjection> lines) const;
    pathfinder::foundation::Result<std::vector<CognitionLineProjection>> buildCognitionLines(std::vector<CognitionLineProjection> lines) const;
};

class ConditionSummaryProjectionBuilder {
public:
    pathfinder::foundation::Result<std::vector<ConditionSummaryProjection>> build(std::vector<ConditionSummaryProjection> conditions) const;
};

class DangerHintProjectionBuilder {
public:
    pathfinder::foundation::Result<std::vector<DangerHintProjection>> build(std::vector<DangerHintProjection> hints) const;
};

class TribeStatusProjectionBuilder {
public:
    pathfinder::foundation::Result<std::optional<TribeStatusProjection>> build(std::optional<TribeStatusProjection> status) const;
};

class ReplayStatusProjectionBuilder {
public:
    pathfinder::foundation::Result<std::optional<ReplayStatusProjection>> build(std::optional<ReplayStatusProjection> status, ProjectionAudience audience) const;
    pathfinder::foundation::Result<ReplayStatusProjection> fromGoldenReplayReport(const pathfinder::replay::GoldenReplayReport& report) const;
};

class H5ProjectionComposer {
public:
    pathfinder::foundation::Result<H5GameProjection> compose(
        const H5ProjectionRequest& request,
        const H5ProjectionSourceBundle& sources) const;

private:
    ProjectionSanitizer sanitizer_;
};

class H5ProtocolProjectionAdapter {
public:
    pathfinder::protocol::ProtocolEnvelope buildProjectionEnvelope(
        const H5GameProjection& projection,
        const H5ProjectionProtocolOptions& options = {}) const;
};

SafeTextProjection makeSafeText(std::string key, SafeTextKind kind, std::string zh_cn);
ProjectionIssue makeProjectionIssue(
    ProjectionIssueKind kind,
    std::string severity_key,
    SafeTextProjection safe_summary,
    bool blocked_output = false);

} // namespace pathfinder::h5_projection
