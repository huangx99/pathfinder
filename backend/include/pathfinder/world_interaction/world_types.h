#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/h5_dialog/dialog_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pathfinder::world_interaction {

enum class WorldObjectInstanceKind {
    Unknown,
    ResourceStack,
    ToolInstance,
    ConsumableInstance,
    GeneratedInstance,
    ThreatMarker,
    CompanionHeldItem,
    TestOnly
};

enum class WorldChangeKind {
    Unknown,
    ObjectQuantityChanged,
    ObjectConsumed,
    ObjectGenerated,
    ObjectStateChanged,
    ActorNeedChanged,
    ActorConditionChanged,
    ThreatLevelChanged,
    ThreatResolved,
    ObjectiveProgressed,
    AgentActionQueued,
    AgentActionExecuted,
    StoryOutcomeReached,
    TestOnly
};

enum class InteractionFailureKind {
    Unknown,
    MissingObject,
    InsufficientQuantity,
    MissingTarget,
    TargetMismatch,
    ConditionNotMet,
    ToolUnavailable,
    KnowledgeNotKnown,
    ThreatNotActive,
    CompanionCannotAct,
    ForbiddenByAudience,
    TestOnly
};

enum class AgentAutonomyActionKind {
    Unknown,
    None,
    HoldTorch,
    GatherMaterial,
    WarnDanger,
    MaintainTool,
    AvoidHazard,
    ApproachTarget,
    AttackTarget,
    FollowActor,
    TeachBack,
    TestOnly
};

enum class ThreatEventPhase {
    Unknown,
    Dormant,
    Foreshadowing,
    Approaching,
    Confronting,
    Mitigated,
    Resolved,
    Failed,
    TestOnly
};

std::string toString(WorldObjectInstanceKind kind);
std::string toString(WorldChangeKind kind);
std::string toString(InteractionFailureKind kind);
std::string toString(AgentAutonomyActionKind kind);
std::string toString(ThreatEventPhase phase);

struct WorldObjectInstance {
    std::string instance_id;
    std::string definition_key;
    std::string display_name_zh_cn;
    WorldObjectInstanceKind kind{WorldObjectInstanceKind::Unknown};
    std::string location_key{"forest_edge"};
    int quantity{0};
    bool visible{true};
    bool usable{true};
    std::vector<std::string> state_tags;
    std::unordered_map<std::string, double> numeric_states;
    std::vector<std::string> source_change_ids;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldActorRuntimeState {
    std::string actor_key;
    std::string display_name_zh_cn;
    bool is_agent_controlled{true};
    bool can_act{true};
    double trust{0.0};
    std::vector<std::string> known_effect_keys;
    std::vector<pathfinder::knowledge::KnowledgeClaim> known_claims;
    std::vector<std::string> observed_object_keys;
    std::vector<std::string> held_object_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldThreatRuntimeState {
    std::string threat_key;
    std::string display_name_zh_cn;
    ThreatEventPhase phase{ThreatEventPhase::Dormant};
    double level{0.0};
    bool active{false};
    bool resolved{false};
    std::string last_change_reason;
    std::vector<std::string> observed_object_keys;
    std::vector<std::string> instinct_effect_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldSnapshot {
    std::string snapshot_id;
    std::string scenario_key;
    uint64_t turn_index{0};
    std::string location_key{"forest_edge"};
    std::unordered_map<std::string, WorldObjectInstance> objects_by_key;
    std::unordered_map<std::string, WorldActorRuntimeState> actors_by_key;
    std::unordered_map<std::string, WorldThreatRuntimeState> threats_by_key;
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldInteractionInput {
    std::string interaction_id;
    std::string actor_key{"pioneer"};
    std::string object_key;
    std::string target_object_key;
    pathfinder::h5_dialog::DialogActionKind action{pathfinder::h5_dialog::DialogActionKind::Unknown};
    std::string effect_key;
    std::string feedback_key;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct InteractionResolveOptions {
    bool allow_player_visible_failures{true};
    bool allow_test_only{false};
};

struct WorldChange {
    std::string change_id;
    WorldChangeKind kind{WorldChangeKind::Unknown};
    std::optional<std::string> target_instance_id;
    std::optional<std::string> target_actor_key;
    std::optional<std::string> target_threat_key;
    std::optional<std::string> definition_key;
    int quantity_delta{0};
    std::optional<std::string> state_key;
    std::optional<double> numeric_delta;
    std::optional<double> numeric_set_value;
    std::vector<std::string> tag_add;
    std::vector<std::string> tag_remove;
    std::string player_summary_zh_cn;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CompanionAssistResult {
    AgentAutonomyActionKind action_kind{AgentAutonomyActionKind::None};
    bool executed{false};
    std::string agent_actor_key;
    std::string required_knowledge_effect_key;
    std::string used_object_key;
    std::string target_key;
    std::string summary_zh_cn;
    std::optional<InteractionFailureKind> skip_reason;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldInteractionResult {
    bool ok{false};
    std::optional<InteractionFailureKind> failure_kind;
    std::vector<WorldChange> changes;
    std::optional<CompanionAssistResult> companion_assist;
    std::optional<std::string> learning_feedback_key;
    std::string player_feedback_zh_cn;
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentAutonomyRequest {
    std::string request_key;
    std::string agent_actor_key;
    std::string trigger_key;
    std::string target_threat_key;
    std::string required_knowledge_effect_key;
    std::vector<std::string> observed_object_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentAutonomyResult {
    AgentAutonomyActionKind action_kind{AgentAutonomyActionKind::None};
    bool executed{false};
    std::string agent_actor_key;
    std::string required_knowledge_effect_key;
    std::string used_object_key;
    std::string target_key;
    std::string summary_zh_cn;
    std::optional<InteractionFailureKind> skip_reason;
    std::vector<WorldChange> changes;
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ThreatProgressInput {
    std::string threat_key{"beast_shadow"};
    double level_delta{25.0};
    uint64_t elapsed_ticks{0};
};

struct ThreatCountermeasureInput {
    std::string threat_key{"beast_shadow"};
    std::string countermeasure_key;
    double level_delta{-35.0};
    std::string summary_zh_cn;
};

struct WorldObjectProjectionPatch {
    std::string object_key;
    std::string status_summary_zh_cn;
    std::vector<std::string> safe_tags;
    bool visible{true};
    bool usable{true};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldProjectionPatch {
    std::vector<std::string> scene_summary_zh_cn;
    std::vector<std::string> player_feedback_lines_zh_cn;
    std::vector<WorldObjectProjectionPatch> object_patches;
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::world_interaction
