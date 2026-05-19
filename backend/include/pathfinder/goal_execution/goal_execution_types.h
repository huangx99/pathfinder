#pragma once

#include "pathfinder/agent_reasoning/reasoning_types.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/world_interaction/world_types.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::goal_execution {

enum class ExecutionFrameStatus {
    Unknown,
    Pending,
    Running,
    Paused,
    Completed,
    Cancelled,
    Failed,
    Expired,
    WaitingForReasoning,
    TestOnly
};

enum class ActionDriverKind {
    Unknown,
    Eat,
    Gather,
    ChopWood,
    SharpenTool,
    BuildStructure,
    UseObject,
    MoveTo,
    CounterThreat,
    AdvancedConstruct,
    TestOnly
};

enum class InternalBlockerKind {
    Unknown,
    MissingObject,
    InsufficientQuantity,
    ToolStateInsufficient,
    ObjectStateInvalid,
    KnowledgeMissing,
    LocationMismatch,
    ResourceDepleted,
    ResourceReserved,
    PathBlocked,
    MissingCondition,
    TestOnly
};

enum class ExternalInterruptKind {
    Unknown,
    ThreatAppeared,
    ThreatEscalated,
    WeatherChanged,
    DependentInDanger,
    ResourceDestroyed,
    ToolLostOrBroken,
    PathBlocked,
    FireSpread,
    SocialConflict,
    DiscoveryFound,
    CommandOverride,
    TestOnly
};

enum class InterruptDecisionKind {
    Ignore,
    ObserveOnly,
    PauseAndInsertEmergencyGoal,
    CancelCurrentGoal,
    ReplanCurrentGoal,
    ResumeAfterHandling,
    FailCurrentGoal,
    TestOnly
};

enum class ResumeDecisionKind {
    Resume,
    Replan,
    Cancel,
    Wait,
    Fail,
    TestOnly
};

enum class MaterialConsumeTiming {
    OnStart,
    OnFinish,
    PerTick,
    ReservationOnly,
    TestOnly
};

enum class ReservationStatus {
    Active,
    Released,
    Consumed,
    Expired,
    TestOnly
};

enum class AgentCapabilityTier {
    InstinctAgent,
    SimpleAgent,
    CognitiveAgent,
    AdvancedAgent,
    TestOnly
};

std::string toString(ExecutionFrameStatus status);
std::string toString(ActionDriverKind kind);
std::string toString(InternalBlockerKind kind);
std::string toString(ExternalInterruptKind kind);
std::string toString(InterruptDecisionKind kind);
std::string toString(ResumeDecisionKind kind);
std::string toString(MaterialConsumeTiming timing);
std::string toString(ReservationStatus status);
std::string toString(AgentCapabilityTier tier);

struct SafeTextProjection {
    std::string text_zh_cn;
    std::vector<std::string> source_trace_keys;
};

struct MaterialRequirement {
    std::string requirement_id;
    std::string object_key;
    int required_quantity{1};
    std::vector<std::string> acceptable_state_tags;
    std::vector<std::string> forbidden_state_tags;
    std::optional<std::string> quality_key;
    std::optional<double> minimum_quality_value;
    MaterialConsumeTiming consume_timing{MaterialConsumeTiming::ReservationOnly};
    bool allow_substitutes{false};
    std::optional<std::string> substitute_group_key;
    std::optional<std::string> source_effect_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MaterialRequirementSet {
    std::string set_id;
    std::string owner_step_id;
    std::vector<MaterialRequirement> requirements;
    bool all_required{true};
    bool allow_partial_progress{false};
    std::string public_summary_zh_cn;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MaterialAvailability {
    std::string object_key;
    int total_quantity{0};
    int reserved_quantity{0};
    int available_quantity{0};
    int matching_quantity{0};
    int missing_quantity{0};
    bool blocked_by_state{false};
    bool blocked_by_reservation{false};
    std::vector<std::string> substitute_candidates;
    std::string safe_summary_zh_cn;
};

struct ResourceReservation {
    std::string reservation_id;
    std::string actor_key;
    std::string resource_key;
    int quantity{0};
    std::string purpose_goal_id;
    uint64_t expires_tick{0};
    ReservationStatus status{ReservationStatus::Active};
};

struct DriverExecutionState {
    std::string driver_state_id;
    ActionDriverKind driver_kind{ActionDriverKind::Unknown};
    std::string action_key;
    std::optional<std::string> object_key;
    std::optional<std::string> target_key;
    std::optional<std::string> current_location_key;
    std::optional<std::string> required_location_key;
    double progress{0.0};
    uint32_t remaining_ticks{1};
    uint32_t attempt_count{0};
    std::vector<std::string> local_memory_keys;
    MaterialRequirementSet material_requirements;
};

struct GoalFrame {
    std::string frame_id;
    std::string actor_key;
    std::string goal_id;
    std::string plan_id;
    std::optional<std::string> parent_frame_id;
    ExecutionFrameStatus status{ExecutionFrameStatus::Pending};
    double priority{0.0};
    uint64_t created_tick{0};
    std::optional<uint64_t> expires_tick;
    ResumeDecisionKind resume_policy{ResumeDecisionKind::Resume};
    std::string public_reason_zh_cn;
    std::vector<std::string> trace_keys;
};

struct PausedExecutionContext {
    std::string paused_context_id;
    std::string frame_id;
    DriverExecutionState driver_state;
    std::string pause_reason_key;
    uint64_t paused_tick{0};
    uint64_t resume_deadline_tick{0};
    std::vector<std::string> resume_validator_keys;
};

struct ExecutionContext {
    std::string context_id;
    std::string actor_key;
    std::vector<GoalFrame> goal_stack;
    std::optional<std::string> active_frame_id;
    std::optional<DriverExecutionState> active_driver_state;
    std::vector<PausedExecutionContext> paused_contexts;
    std::vector<ResourceReservation> reserved_resources;
    std::vector<pathfinder::knowledge::KnowledgeClaim> known_claims;
    AgentCapabilityTier capability_tier{AgentCapabilityTier::SimpleAgent};
    uint64_t last_update_tick{0};
    std::vector<std::string> trace_keys;
};

struct DriverCommand {
    std::string command_id;
    std::string actor_key;
    ActionDriverKind driver_kind{ActionDriverKind::Unknown};
    std::optional<std::string> object_key;
    std::optional<std::string> target_key;
    std::string action_key;
    std::optional<std::string> effect_key;
    uint32_t estimated_ticks{1};
    bool requires_world_resolution{true};
    std::string public_summary_zh_cn;
};

struct InternalBlocker {
    std::string blocker_id;
    InternalBlockerKind kind{InternalBlockerKind::Unknown};
    std::string actor_key;
    std::string source_driver_state_id;
    std::string missing_key;
    std::string required_value;
    bool can_generate_subgoal{false};
    std::optional<pathfinder::agent_reasoning::AgentGoalKind> suggested_goal_kind;
    std::string safe_summary_zh_cn;
};

struct ExecutionFeedback {
    std::string feedback_id;
    std::string safe_summary_zh_cn;
    std::vector<std::string> learning_signal_keys;
    std::vector<std::string> trace_keys;
};

struct DriverTickResult {
    ExecutionFrameStatus status{ExecutionFrameStatus::Running};
    double progress_delta{0.0};
    std::vector<DriverCommand> commands;
    std::vector<InternalBlocker> internal_blockers;
    std::vector<ExecutionFeedback> generated_feedback;
    bool should_replan{false};
    std::string safe_summary_zh_cn;
    std::vector<std::string> trace_keys;
};

struct ExternalInterruptSignal {
    std::string interrupt_id;
    ExternalInterruptKind kind{ExternalInterruptKind::Unknown};
    std::string source_event_id;
    std::optional<std::string> location_key;
    std::vector<std::string> target_actor_keys;
    std::optional<std::string> threat_key;
    double severity{0.0};
    double urgency{0.0};
    bool can_be_ignored{true};
    std::string public_summary_zh_cn;
    std::vector<std::string> reason_keys;
};

struct InterruptDecision {
    std::string decision_id;
    InterruptDecisionKind kind{InterruptDecisionKind::Ignore};
    std::string actor_key;
    std::string interrupt_id;
    std::optional<std::string> affected_frame_id;
    std::optional<pathfinder::agent_reasoning::AgentGoal> inserted_goal_request;
    bool pause_current{false};
    bool cancel_current{false};
    bool requires_replan{false};
    std::string safe_explanation_zh_cn;
    std::vector<std::string> trace_keys;
};

struct ResumeDecision {
    ResumeDecisionKind kind{ResumeDecisionKind::Resume};
    std::string safe_summary_zh_cn;
    std::vector<std::string> trace_keys;
};

struct DriverTickInput {
    std::string request_id;
    std::string actor_key;
    pathfinder::world_interaction::WorldSnapshot world_snapshot;
    ExecutionContext execution_context;
    GoalFrame active_frame;
    DriverExecutionState driver_state;
    uint32_t elapsed_ticks{1};
    std::vector<ExternalInterruptSignal> visible_events;
};

struct MaterialEvaluationInput {
    pathfinder::world_interaction::WorldSnapshot world_snapshot;
    std::vector<ResourceReservation> reserved_resources;
    MaterialRequirementSet requirements;
};

struct MaterialEvaluationResult {
    std::string evaluation_id;
    bool satisfied{false};
    std::vector<MaterialAvailability> availability;
    std::string safe_summary_zh_cn;
};

struct ReactionMaterialLink {
    std::string link_id;
    std::string requirement_id;
    std::string reaction_rule_id;
    std::vector<std::string> input_object_keys;
    std::string output_object_key;
    int expected_output_quantity{1};
    std::string condition_expression;
};

struct MaterialReserveRequest {
    std::string actor_key;
    std::string resource_key;
    int quantity{0};
    std::string purpose_goal_id;
    uint64_t expires_tick{0};
};

struct InternalBlockerResolveInput {
    ExecutionContext context;
    std::vector<InternalBlocker> blockers;
};

struct InterruptEvaluationInput {
    std::vector<ExternalInterruptSignal> signals;
    ExecutionContext context;
    pathfinder::world_interaction::WorldSnapshot world_snapshot;
};

struct ExecutionTickRequest {
    std::string request_id;
    ExecutionContext context;
    pathfinder::world_interaction::WorldSnapshot world_snapshot;
    std::vector<ExternalInterruptSignal> visible_events;
    std::optional<pathfinder::agent_reasoning::AgentPlan> incoming_plan;
    uint32_t elapsed_ticks{1};
    uint64_t tick{0};
};

struct ExecutionStatusProjection {
    SafeTextProjection current_goal;
    SafeTextProjection active_step;
    SafeTextProjection blocked_by;
    SafeTextProjection interrupt_reason;
    SafeTextProjection response_plan;
    SafeTextProjection resume_hint;
    std::vector<SafeTextProjection> trace_lines;
};

struct ExecutionResult {
    bool ok{false};
    ExecutionContext updated_context;
    std::vector<DriverCommand> commands_to_resolve;
    std::vector<pathfinder::world_interaction::WorldInteractionInput> world_changes;
    std::vector<InterruptDecision> interrupt_decisions;
    std::vector<InternalBlocker> internal_blockers;
    std::vector<pathfinder::agent_reasoning::AgentGoal> generated_subgoals;
    std::vector<ExecutionFeedback> execution_feedback;
    bool requires_reasoning{false};
    pathfinder::agent_reasoning::ReasoningRequest reasoning_request;
    ExecutionStatusProjection projection;
    std::string safe_summary_zh_cn;
    std::vector<std::string> trace;
};

} // namespace pathfinder::goal_execution
