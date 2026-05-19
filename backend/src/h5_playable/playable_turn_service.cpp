#include "pathfinder/h5_playable/playable_turn_service.h"
#include "pathfinder/goal_execution/goal_execution_system.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include "pathfinder/world_interaction/world_services.h"
#include <algorithm>

namespace pathfinder::h5_playable {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using namespace pathfinder::h5_projection;

namespace {

bool hasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::string fieldFromKey(const std::vector<std::string>& keys, const std::string& prefix, const std::string& fallback) {
    for (const auto& key : keys) {
        if (hasPrefix(key, prefix)) return key.substr(prefix.size());
    }
    return fallback;
}

int intFieldFromKey(const std::vector<std::string>& keys, const std::string& prefix, int fallback) {
    const auto text = fieldFromKey(keys, prefix, "");
    if (text.empty()) return fallback;
    try {
        return std::stoi(text);
    } catch (...) {
        return fallback;
    }
}

bool containsKey(const std::vector<std::string>& keys, const std::string& value) {
    return std::find(keys.begin(), keys.end(), value) != keys.end();
}

std::string scenarioThreatCounterEffectKey(const std::string& threat_object_key) {
    pathfinder::h5_dialog::DialogScenarioCatalog catalog;
    auto scenario = catalog.defaultScenario();
    if (scenario.is_error()) return "";
    for (const auto& config : scenario.value().threat_knowledge_templates) {
        if (config.threat_object_key == threat_object_key) return config.required_effect_key;
    }
    return "";
}

std::string scenarioFeedbackEffectKey(const std::string& object_key, const std::string& target_object_key, pathfinder::h5_dialog::DialogActionKind action) {
    pathfinder::h5_dialog::DialogScenarioCatalog catalog;
    auto scenario = catalog.defaultScenario();
    if (scenario.is_error()) return "";
    for (const auto& feedback : scenario.value().feedbacks) {
        if (feedback.object_key == object_key && feedback.target_object_key == target_object_key && feedback.action == action) return feedback.effect_key;
    }
    return "";
}

void upsertKey(std::vector<std::string>& keys, const std::string& prefix, const std::string& value) {
    keys.erase(std::remove_if(keys.begin(), keys.end(), [&](const std::string& key) { return hasPrefix(key, prefix); }), keys.end());
    keys.push_back(prefix + value);
}

void addUnique(std::vector<std::string>& keys, const std::string& value) {
    if (!containsKey(keys, value)) keys.push_back(value);
}

pathfinder::story::StoryEvaluationContext buildStoryContext(
    const pathfinder::h5_dialog::DialogResponseDto& dialog_response,
    const pathfinder::h5_dialog::DialogSessionState& session_state,
    const pathfinder::h5_dialog::DialogIntent& turn_intent,
    const std::string& observed_effect_key) {
    pathfinder::story::StoryEvaluationContext context;
    context.last_input_text = turn_intent.normalized_text.empty() ? dialog_response.reply_text : turn_intent.normalized_text;
    context.last_intent_kind = pathfinder::h5_dialog::toString(turn_intent.kind);
    context.last_action_key = pathfinder::h5_dialog::toString(turn_intent.action);
    context.last_object_key = turn_intent.object_key;
    context.last_target_object_key = turn_intent.target_object_key;
    context.last_effect_key = observed_effect_key;
    for (const auto& claim : session_state.actor_claims) {
        if (!claim.predicate.effect_key.empty()) context.actor_knowledge_effect_keys.push_back(claim.predicate.effect_key);
    }
    for (const auto& claim : session_state.recipient_claims) {
        if (!claim.predicate.effect_key.empty()) context.recipient_knowledge_effect_keys.push_back(claim.predicate.effect_key);
    }
    context.available_object_keys = session_state.visible_object_keys;
    for (const auto& [object_key, runtime] : session_state.object_runtime_states) {
        auto quantity = runtime.numeric_states.find("quantity");
        if (quantity != runtime.numeric_states.end() && quantity->second > 0.0) addUnique(context.available_object_keys, object_key);
    }
    context.completed_action_keys = session_state.completed_action_keys;
    context.teach_action_happened = dialog_response.decision == pathfinder::h5_dialog::DialogTurnDecision::TeachingRan;
    return context;
}

pathfinder::story::StoryRuntimeState storyStateFromSession(
    const pathfinder::story::StoryScenarioDefinition& scenario,
    const pathfinder::story::StoryRuntimeFactory& factory,
    const pathfinder::h5_dialog::DialogSessionState& session_state) {
    auto initial = factory.createInitialState(scenario);
    auto state = initial.is_ok() ? initial.value() : pathfinder::story::StoryRuntimeState{};
    state.current_phase = pathfinder::story::storyTimePhaseFromString(fieldFromKey(session_state.debug_keys, "story.phase:", "morning")).value();
    state.time_points_used = intFieldFromKey(session_state.debug_keys, "story.time:", 0);
    state.turn_index = static_cast<uint64_t>(intFieldFromKey(session_state.debug_keys, "story.turn:", static_cast<int>(session_state.turn_index)));
    for (auto& threat : state.threat_states) {
        if (containsKey(session_state.debug_keys, "story.threat." + threat.threat_key + ".active")) threat.active = true;
        if (containsKey(session_state.debug_keys, "story.threat." + threat.threat_key + ".resolved")) threat.resolved = true;
        if (containsKey(session_state.debug_keys, "story.threat." + threat.threat_key + ".mitigated")) threat.mitigated = true;
    }
    for (const auto& key : session_state.debug_keys) {
        if (hasPrefix(key, "story.generated:")) state.generated_object_keys.push_back(key.substr(std::string("story.generated:").size()));
    }
    return state;
}

void persistStoryState(pathfinder::h5_dialog::DialogSessionState& session_state, const pathfinder::story::StoryRuntimeState& state) {
    upsertKey(session_state.debug_keys, "story.phase:", pathfinder::story::toString(state.current_phase));
    upsertKey(session_state.debug_keys, "story.time:", std::to_string(state.time_points_used));
    upsertKey(session_state.debug_keys, "story.turn:", std::to_string(state.turn_index));
    session_state.debug_keys.erase(std::remove_if(session_state.debug_keys.begin(), session_state.debug_keys.end(), [](const std::string& key) {
        return hasPrefix(key, "story.threat.") || hasPrefix(key, "story.generated:");
    }), session_state.debug_keys.end());
    for (const auto& threat : state.threat_states) {
        if (threat.active) addUnique(session_state.debug_keys, "story.threat." + threat.threat_key + ".active");
        if (threat.resolved) addUnique(session_state.debug_keys, "story.threat." + threat.threat_key + ".resolved");
        if (threat.mitigated) addUnique(session_state.debug_keys, "story.threat." + threat.threat_key + ".mitigated");
    }
    for (const auto& object_key : state.generated_object_keys) addUnique(session_state.debug_keys, "story.generated:" + object_key);
}

void applyStoryProjection(pathfinder::h5_projection::H5GameProjection& game_projection,
                          const pathfinder::story::StoryProjection& story_projection) {
    game_projection.scene_summary.insert(game_projection.scene_summary.begin(), story_projection.main_objective);
    game_projection.scene_summary.insert(game_projection.scene_summary.begin(), story_projection.time_phase_label);
    for (const auto& card : story_projection.objective_cards) {
        game_projection.scene_summary.push_back(card.title);
        game_projection.scene_summary.push_back(card.hint);
    }
    for (const auto& hint : story_projection.material_hints) game_projection.scene_summary.push_back(hint);
    for (const auto& warning : story_projection.threat_warnings) {
        pathfinder::h5_projection::DangerHintProjection danger;
        danger.danger_key = "story.threat.approaching_beast";
        danger.severity_label = pathfinder::h5_projection::makeSafeText("story.threat.severity", pathfinder::h5_projection::SafeTextKind::Warning, "正在靠近");
        danger.hint_text = warning;
        danger.countermeasure_labels.push_back(pathfinder::h5_projection::makeSafeText("story.threat.counter.fire", pathfinder::h5_projection::SafeTextKind::Hint, "火光、火堆或火把可能有帮助。"));
        game_projection.danger_hints.push_back(std::move(danger));
    }
    for (const auto& action : story_projection.suggested_actions) game_projection.action_bar.push_back(action);
}

PlayableFeedbackTone toneFromDialog(const pathfinder::h5_dialog::DialogResponseDto& response, const pathfinder::h5_dialog::DialogIntent& intent, const std::string& observed_effect_key) {
    if (response.decision == pathfinder::h5_dialog::DialogTurnDecision::Rejected ||
        response.decision == pathfinder::h5_dialog::DialogTurnDecision::Failed) return PlayableFeedbackTone::Warning;
    if (observed_effect_key == "poison" || intent.object_key == "beast_shadow" || intent.target_object_key == "beast_shadow") return PlayableFeedbackTone::Danger;
    if (response.decision == pathfinder::h5_dialog::DialogTurnDecision::LearningLoopRan ||
        response.decision == pathfinder::h5_dialog::DialogTurnDecision::TeachingRan ||
        observed_effect_key == "restore_hunger" ||
        observed_effect_key == "make_torch" ||
        observed_effect_key == "ignite_fire" ||
        observed_effect_key == "repel_beast") return PlayableFeedbackTone::Positive;
    return PlayableFeedbackTone::Neutral;
}


pathfinder::h5_projection::SafeTextProjection executionText(const std::string& key, const std::string& text) {
    return pathfinder::h5_projection::makeSafeText(key, pathfinder::h5_projection::SafeTextKind::Feedback, text);
}

H5PlayableExecutionStatus toPlayableExecutionStatus(const pathfinder::goal_execution::ExecutionStatusProjection& projection) {
    auto convert = [](const pathfinder::goal_execution::SafeTextProjection& text_value, const std::string& key) -> std::optional<pathfinder::h5_projection::SafeTextProjection> {
        if (text_value.text_zh_cn.empty()) return std::nullopt;
        auto safe = executionText("execution." + key, text_value.text_zh_cn);
        safe.reason_keys = text_value.source_trace_keys;
        return safe;
    };
    H5PlayableExecutionStatus status;
    status.current_goal = convert(projection.current_goal, "current_goal");
    status.active_step = convert(projection.active_step, "active_step");
    status.blocked_by = convert(projection.blocked_by, "blocked_by");
    status.interrupt_reason = convert(projection.interrupt_reason, "interrupt_reason");
    status.response_plan = convert(projection.response_plan, "response_plan");
    status.resume_hint = convert(projection.resume_hint, "resume_hint");
    for (const auto& line : projection.trace_lines) {
        auto converted = convert(line, "trace");
        if (converted.has_value()) status.trace_lines.push_back(*converted);
    }
    status.visible = status.current_goal.has_value() || status.active_step.has_value() || status.blocked_by.has_value() || status.interrupt_reason.has_value();
    return status;
}

// TODO(P41-execution-persistence): This builds a minimal visible plan for H5 P40 projection.
// Long-running goals should persist ExecutionContext in the session/save system once map and scheduling stages land.
pathfinder::agent_reasoning::AgentPlan playableExecutionPlan(
    const pathfinder::h5_dialog::DialogSessionState& session_state,
    const pathfinder::h5_dialog::DialogIntent& turn_intent,
    const std::string& observed_effect_key) {
    pathfinder::agent_reasoning::AgentPlan plan;
    plan.plan_id = "plan.h5_visible_execution." + session_state.session_id + "." + std::to_string(session_state.turn_index);
    plan.actor_key = "companion";
    plan.goal.goal_id = "goal.h5_visible_execution." + std::to_string(session_state.turn_index);
    plan.goal.actor_key = "companion";
    plan.goal.kind = pathfinder::agent_reasoning::AgentGoalKind::AcquireObject;
    plan.goal.target_key = "camp_progress";
    plan.goal.urgency = 35.0;
    plan.goal.source_keys = {"h5_turn"};

    pathfinder::agent_reasoning::AgentPlanStep step;
    step.step_id = "step.h5_visible_execution." + std::to_string(session_state.turn_index);
    step.actor_key = "companion";
    step.estimated_ticks = 1;
    step.explanation_zh_cn = "同伴正在根据当前局势行动。";

    const bool beast_related = turn_intent.object_key == "beast_shadow" ||
        turn_intent.target_object_key == "beast_shadow" ||
        observed_effect_key == scenarioThreatCounterEffectKey("beast_shadow") ||
        containsKey(session_state.debug_keys, "story.threat.beast_shadow.active");
    if (beast_related) {
        plan.goal.kind = pathfinder::agent_reasoning::AgentGoalKind::ReduceThreat;
        plan.goal.target_key = "beast_shadow";
        plan.goal.urgency = 85.0;
        step.kind = pathfinder::agent_reasoning::PlanStepKind::DirectAction;
        step.object_key = "torch";
        step.target_key = "beast_shadow";
        step.action_key = "use";
        step.effect_key = scenarioThreatCounterEffectKey("beast_shadow");
        if (step.effect_key.empty()) step.effect_key = "use";
        step.explanation_zh_cn = "同伴正在盯住靠近的危险。";
    } else if ((turn_intent.object_key == "axe" && turn_intent.target_object_key == "wood") || observed_effect_key == scenarioFeedbackEffectKey("axe", "wood", pathfinder::h5_dialog::DialogActionKind::Use)) {
        plan.goal.kind = pathfinder::agent_reasoning::AgentGoalKind::AcquireObject;
        plan.goal.target_key = "wood_processed";
        step.kind = pathfinder::agent_reasoning::PlanStepKind::DirectAction;
        step.object_key = "axe";
        step.target_key = "wood";
        step.action_key = "use";
        step.effect_key = scenarioFeedbackEffectKey("axe", "wood", pathfinder::h5_dialog::DialogActionKind::Use);
        if (step.effect_key.empty()) step.effect_key = "use";
        step.explanation_zh_cn = "同伴正在为营地准备木材。";
    } else {
        step.kind = pathfinder::agent_reasoning::PlanStepKind::WaitForCondition;
        step.action_key = "wait";
        step.effect_key = "wait";
        step.explanation_zh_cn = "同伴正在观察局势，等待下一步。";
    }
    plan.steps.push_back(step);
    plan.total_estimated_ticks = 1;
    plan.trace_keys = {"h5_execution_projection"};
    return plan;
}

std::vector<pathfinder::goal_execution::ExternalInterruptSignal> playableInterrupts(
    const pathfinder::h5_dialog::DialogSessionState& session_state,
    const pathfinder::h5_dialog::DialogIntent& turn_intent,
    const std::string& observed_effect_key) {
    const bool beast_related = turn_intent.object_key == "beast_shadow" ||
        turn_intent.target_object_key == "beast_shadow" ||
        observed_effect_key == scenarioThreatCounterEffectKey("beast_shadow") ||
        containsKey(session_state.debug_keys, "story.threat.beast_shadow.active");
    if (!beast_related) return {};
    pathfinder::goal_execution::ExternalInterruptSignal signal;
    signal.interrupt_id = "interrupt.h5.beast_shadow." + std::to_string(session_state.turn_index);
    signal.kind = pathfinder::goal_execution::ExternalInterruptKind::ThreatAppeared;
    signal.source_event_id = "event.h5.beast_shadow";
    signal.location_key = "forest_edge";
    signal.target_actor_keys = {"companion"};
    signal.threat_key = "beast_shadow";
    signal.severity = 90.0;
    signal.urgency = 90.0;
    signal.can_be_ignored = false;
    signal.public_summary_zh_cn = "靠近的野兽打断了同伴原本的行动。";
    return {signal};
}

Result<H5PlayableExecutionStatus> buildExecutionStatus(
    const pathfinder::h5_dialog::DialogSessionState& session_state,
    const pathfinder::h5_dialog::DialogIntent& turn_intent,
    const std::string& observed_effect_key) {
    pathfinder::h5_dialog::DialogScenarioCatalog catalog;
    auto scenario = catalog.defaultScenario();
    if (scenario.is_error()) return Result<H5PlayableExecutionStatus>::fail(scenario.errors());
    pathfinder::world_interaction::WorldSnapshotAdapter snapshot_adapter;
    auto snapshot = snapshot_adapter.fromDialogSession(scenario.value(), session_state);
    if (snapshot.is_error()) return Result<H5PlayableExecutionStatus>::fail(snapshot.errors());

    pathfinder::goal_execution::ExecutionContext context;
    context.context_id = "execution.h5." + session_state.session_id;
    context.actor_key = "companion";
    context.capability_tier = pathfinder::goal_execution::AgentCapabilityTier::CognitiveAgent;
    auto actor = snapshot.value().actors_by_key.find("companion");
    if (actor != snapshot.value().actors_by_key.end()) context.known_claims = actor->second.known_claims;

    pathfinder::goal_execution::ExecutionTickRequest request;
    request.request_id = "execution_tick.h5." + session_state.session_id + "." + std::to_string(session_state.turn_index);
    request.context = std::move(context);
    request.world_snapshot = snapshot.value();
    request.visible_events = playableInterrupts(session_state, turn_intent, observed_effect_key);
    request.incoming_plan = playableExecutionPlan(session_state, turn_intent, observed_effect_key);
    request.elapsed_ticks = 1;
    request.tick = session_state.turn_index;

    pathfinder::goal_execution::GoalExecutionSystem system;
    auto executed = system.tick(request);
    if (executed.is_error()) return Result<H5PlayableExecutionStatus>::fail(executed.errors());
    auto status = toPlayableExecutionStatus(executed.value().projection);
    if (!status.current_goal.has_value()) status.current_goal = executionText("execution.current_goal.fallback", "同伴正在执行当前计划。");
    if (!status.active_step.has_value()) status.active_step = executionText("execution.active_step.fallback", "当前步骤：等待或观察。完成后会继续评估目标。");
    status.visible = true;
    return Result<H5PlayableExecutionStatus>::ok(std::move(status));
}

pathfinder::h5_dialog::DialogResponseDto bootstrapDialogResponse(const std::string& session_id, uint64_t turn_index) {
    pathfinder::h5_dialog::DialogResponseDto response;
    response.session_id = session_id;
    response.decision = pathfinder::h5_dialog::DialogTurnDecision::RepliedOnly;
    response.reply_text = "你醒来了。周围的一切都还等待你理解。";
    response.state_summary_lines.push_back("回合：" + std::to_string(turn_index));
    response.state_summary_lines.push_back("当前地点：林地边缘");
    return response;
}

} // namespace

Result<H5PlayableResponse> H5PlayableTurnService::bootstrap(const H5PlayableBootstrapRequest& request) {
    auto request_result = request.validateBasic();
    if (request_result.is_error()) return Result<H5PlayableResponse>::fail(request_result.errors());
    if (request.reset) {
        auto reset_result = dialog_service_.resetSession(request.session_id);
        if (reset_result.is_error()) return Result<H5PlayableResponse>::fail(reset_result.errors());
    }
    auto state_result = dialog_service_.loadOrCreateSessionSnapshot(request.session_id);
    if (state_result.is_error()) return Result<H5PlayableResponse>::fail(state_result.errors());
    auto state = state_result.value();
    auto dialog_response = bootstrapDialogResponse(request.session_id, state.turn_index);
    return buildResponse(dialog_response, state, pathfinder::h5_dialog::DialogIntent{}, "", PlayableFeedbackTone::System, request.reset ? "reset" : "bootstrap");
}

Result<H5PlayableResponse> H5PlayableTurnService::handleTurn(const H5PlayableRequest& request) {
    auto request_result = request.validateBasic();
    if (request_result.is_error()) return Result<H5PlayableResponse>::fail(request_result.errors());

    if (request.input_kind == PlayableInputKind::Reset) {
        H5PlayableBootstrapRequest bootstrap_request;
        bootstrap_request.session_id = request.session_id;
        bootstrap_request.reset = true;
        return bootstrap(bootstrap_request);
    }

    pathfinder::h5_dialog::DialogRequestDto dialog_request;
    dialog_request.session_id = request.session_id;
    dialog_request.client_turn = request.client_turn;
    dialog_request.input_text = request.input_text;
    if (dialog_request.input_text.empty() && request.action_key.has_value()) {
        dialog_request.input_text = *request.action_key;
    }

    auto detailed_result = dialog_service_.handleDetailed(dialog_request);
    if (detailed_result.is_error()) return Result<H5PlayableResponse>::fail(detailed_result.errors());
    const auto detailed = detailed_result.value();
    return buildResponse(detailed.response, detailed.state, detailed.intent, detailed.observed_effect_key, toneFromDialog(detailed.response, detailed.intent, detailed.observed_effect_key), "turn_" + std::to_string(request.client_turn));
}

Result<H5PlayableResponse> H5PlayableTurnService::buildResponse(
    const pathfinder::h5_dialog::DialogResponseDto& dialog_response,
    const pathfinder::h5_dialog::DialogSessionState& session_state,
    const pathfinder::h5_dialog::DialogIntent& turn_intent,
    const std::string& observed_effect_key,
    PlayableFeedbackTone tone,
    const std::string& request_key) {
    auto source_result = mapper_.buildSourceBundle(dialog_response, session_state);
    if (source_result.is_error()) return Result<H5PlayableResponse>::fail(source_result.errors());

    H5ProjectionRequest projection_request;
    projection_request.request_id = request_key + "_" + session_state.session_id;
    projection_request.session_id = session_state.session_id;
    projection_request.audience = ProjectionAudience::Player;
    projection_request.mode = ProjectionMode::Full;
    projection_request.max_items_per_section = 64;
    projection_request.language = "zh_cn";

    auto projection_result = composer_.compose(projection_request, source_result.value());
    if (projection_result.is_error()) return Result<H5PlayableResponse>::fail(projection_result.errors());

    auto scenario_result = story_registry_.firstDaySurvival();
    if (scenario_result.is_error()) return Result<H5PlayableResponse>::fail(scenario_result.errors());
    auto story_state = storyStateFromSession(scenario_result.value(), story_factory_, session_state);
    auto story_context = buildStoryContext(dialog_response, session_state, turn_intent, observed_effect_key);
    auto progressed_story = story_progression_.applyTurn(scenario_result.value(), story_state, story_context);
    if (progressed_story.is_error()) return Result<H5PlayableResponse>::fail(progressed_story.errors());
    auto story_session_state = session_state;
    persistStoryState(story_session_state, progressed_story.value().new_state);
    auto save_story_state = dialog_service_.saveSessionSnapshot(story_session_state);
    if (save_story_state.is_error()) return Result<H5PlayableResponse>::fail(save_story_state.errors());
    auto story_projection_result = story_projection_.composeProjection(scenario_result.value(), progressed_story.value().new_state, story_context, ProjectionAudience::Player);
    if (story_projection_result.is_error()) return Result<H5PlayableResponse>::fail(story_projection_result.errors());

    H5PlayableResponse response;
    response.ok = dialog_response.decision != pathfinder::h5_dialog::DialogTurnDecision::Failed;
    response.session_id = story_session_state.session_id;
    response.server_turn = story_session_state.turn_index;
    response.tone = tone;
    response.reply_text = makeSafeText("playable.reply." + request_key, SafeTextKind::Feedback, playableSafeReplyText(dialog_response));
    response.projection = std::move(projection_result.value());
    applyStoryProjection(response.projection, story_projection_result.value());
    auto execution_status = buildExecutionStatus(story_session_state, turn_intent, observed_effect_key);
    if (execution_status.is_error()) return Result<H5PlayableResponse>::fail(execution_status.errors());
    response.execution_status = execution_status.value();
    response.issues = response.projection.issues;

    auto valid = response.validateBasic();
    if (valid.is_error()) return Result<H5PlayableResponse>::fail(valid.errors());
    return Result<H5PlayableResponse>::ok(std::move(response));
}

} // namespace pathfinder::h5_playable
