#include "pathfinder/h5_playable/playable_turn_service.h"
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

void upsertKey(std::vector<std::string>& keys, const std::string& prefix, const std::string& value) {
    keys.erase(std::remove_if(keys.begin(), keys.end(), [&](const std::string& key) { return hasPrefix(key, prefix); }), keys.end());
    keys.push_back(prefix + value);
}

void addUnique(std::vector<std::string>& keys, const std::string& value) {
    if (!containsKey(keys, value)) keys.push_back(value);
}

pathfinder::story::StoryEvaluationContext buildStoryContext(
    const pathfinder::h5_dialog::DialogResponseDto& dialog_response,
    const pathfinder::h5_dialog::DialogSessionState& session_state) {
    pathfinder::story::StoryEvaluationContext context;
    context.last_input_text = dialog_response.reply_text;
    for (const auto& claim : session_state.actor_claims) {
        if (!claim.predicate.effect_key.empty()) context.actor_knowledge_effect_keys.push_back(claim.predicate.effect_key);
    }
    for (const auto& claim : session_state.recipient_claims) {
        if (!claim.predicate.effect_key.empty()) context.recipient_knowledge_effect_keys.push_back(claim.predicate.effect_key);
    }
    context.available_object_keys = session_state.visible_object_keys;
    context.completed_action_keys = session_state.completed_action_keys;
    context.teach_action_happened = dialog_response.decision == pathfinder::h5_dialog::DialogTurnDecision::TeachingRan || dialog_response.reply_text.find("同伴听取") != std::string::npos;
    if (dialog_response.reply_text.find("火把") != std::string::npos) context.available_object_keys.push_back("torch");
    if (dialog_response.reply_text.find("火") != std::string::npos) context.available_object_keys.push_back("camp_fire");
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

PlayableFeedbackTone toneFromDialog(const pathfinder::h5_dialog::DialogResponseDto& response) {
    if (response.decision == pathfinder::h5_dialog::DialogTurnDecision::Rejected ||
        response.decision == pathfinder::h5_dialog::DialogTurnDecision::Failed) return PlayableFeedbackTone::Warning;
    if (response.reply_text.find("不适") != std::string::npos || response.reply_text.find("危险") != std::string::npos) return PlayableFeedbackTone::Danger;
    if (response.reply_text.find("形成") != std::string::npos || response.reply_text.find("缓解") != std::string::npos || response.reply_text.find("同伴听取") != std::string::npos) return PlayableFeedbackTone::Positive;
    return PlayableFeedbackTone::Neutral;
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
    return buildResponse(dialog_response, state, PlayableFeedbackTone::System, request.reset ? "reset" : "bootstrap");
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
    return buildResponse(detailed.response, detailed.state, toneFromDialog(detailed.response), "turn_" + std::to_string(request.client_turn));
}

Result<H5PlayableResponse> H5PlayableTurnService::buildResponse(
    const pathfinder::h5_dialog::DialogResponseDto& dialog_response,
    const pathfinder::h5_dialog::DialogSessionState& session_state,
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
    auto story_context = buildStoryContext(dialog_response, session_state);
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
    response.issues = response.projection.issues;

    auto valid = response.validateBasic();
    if (valid.is_error()) return Result<H5PlayableResponse>::fail(valid.errors());
    return Result<H5PlayableResponse>::ok(std::move(response));
}

} // namespace pathfinder::h5_playable
