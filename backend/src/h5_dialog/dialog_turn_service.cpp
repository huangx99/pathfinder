#include "pathfinder/h5_dialog/dialog_turn_service.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::learning::LearningLoopOptions;
using pathfinder::learning::LearningLoopInput;
using pathfinder::learning::LearningLoopResult;
using pathfinder::learning::LearningDebugReport;


namespace {

constexpr uint64_t kShortExposureWindowTicks = 6;
constexpr uint64_t kWaitAdvanceTicks = 8;
constexpr size_t kMaxCausalExposureHistory = 128;

bool containsKey(const std::vector<std::string>& keys, const std::string& key) {
    return std::find(keys.begin(), keys.end(), key) != keys.end();
}

const DialogScenarioObject* findScenarioObjectByKey(const DialogScenario& scenario, const std::string& object_key) {
    for (const auto& object : scenario.objects) {
        if (object.object_key == object_key) return &object;
    }
    return nullptr;
}

std::string exposureFamilyForObject(const DialogScenario& scenario, const std::string& object_key) {
    const auto* object = findScenarioObjectByKey(scenario, object_key);
    if (!object) return "";
    for (const auto& tag : object->safe_tags) {
        if (tag == "decayed" || tag == "fresh" || tag == "red" || tag == "green") continue;
        return tag;
    }
    return object->object_key;
}

std::vector<std::string> exposureStatesForObject(const DialogScenario& scenario, const std::string& object_key) {
    const auto* object = findScenarioObjectByKey(scenario, object_key);
    if (!object) return {};
    std::vector<std::string> states;
    if (containsKey(object->safe_tags, "decayed")) {
        states.push_back("decayed");
    } else if (containsKey(object->safe_tags, "fruit") || containsKey(object->safe_tags, "leaf")) {
        states.push_back("fresh");
    }
    return states;
}

bool isRecentExposure(const DialogCausalExposureRecord& exposure,
                      const std::string& family_key,
                      const std::string& action_key,
                      uint64_t now_tick,
                      uint64_t window_ticks) {
    if (exposure.family_key != family_key || exposure.action_key != action_key) return false;
    if (exposure.observed_tick > now_tick) return false;
    return now_tick - exposure.observed_tick <= window_ticks;
}

bool hasFreshLikeExposure(const DialogCausalExposureRecord& exposure) {
    return !containsKey(exposure.state_keys, "decayed");
}

DialogObjectRuntimeState& ensureRuntimeState(DialogSessionState& state, const DialogScenario& scenario, const std::string& object_key) {
    auto it = state.object_runtime_states.find(object_key);
    if (it != state.object_runtime_states.end()) return it->second;

    DialogObjectRuntimeState runtime;
    runtime.object_key = object_key;
    const auto* object = findScenarioObjectByKey(scenario, object_key);
    if (object) runtime.tag_states = object->safe_tags;
    auto inserted = state.object_runtime_states.emplace(object_key, std::move(runtime));
    return inserted.first->second;
}

const DialogObjectRuntimeState* findRuntimeState(const DialogSessionState& state, const std::string& object_key) {
    auto it = state.object_runtime_states.find(object_key);
    if (it == state.object_runtime_states.end()) return nullptr;
    return &it->second;
}

bool hasRuntimeTag(const DialogObjectRuntimeState& runtime, const std::string& tag) {
    return containsKey(runtime.tag_states, tag);
}

void addRuntimeTag(DialogObjectRuntimeState& runtime, const std::string& tag) {
    if (!containsKey(runtime.tag_states, tag)) runtime.tag_states.push_back(tag);
}

void removeRuntimeTag(DialogObjectRuntimeState& runtime, const std::string& tag) {
    runtime.tag_states.erase(std::remove(runtime.tag_states.begin(), runtime.tag_states.end(), tag), runtime.tag_states.end());
}

double runtimeNumber(const DialogSessionState& state, const std::string& object_key, const std::string& state_key) {
    const auto* runtime = findRuntimeState(state, object_key);
    if (!runtime) return 0.0;
    auto it = runtime->numeric_states.find(state_key);
    if (it == runtime->numeric_states.end()) return 0.0;
    return it->second;
}

bool compareNumber(double lhs, const std::string& op, double rhs) {
    if (op == "gt") return lhs > rhs;
    if (op == "gte") return lhs >= rhs;
    if (op == "lt") return lhs < rhs;
    if (op == "lte") return lhs <= rhs;
    if (op == "eq") return std::fabs(lhs - rhs) < 0.000001;
    if (op == "neq") return std::fabs(lhs - rhs) >= 0.000001;
    return false;
}

bool stateConditionMatches(const DialogSessionState& state, const DialogStateCondition& condition) {
    const auto* runtime = findRuntimeState(state, condition.object_key);
    if (condition.op == "has_tag") {
        return runtime && hasRuntimeTag(*runtime, condition.tag_value.empty() ? condition.state_key : condition.tag_value);
    }
    if (condition.op == "missing_tag") {
        return !runtime || !hasRuntimeTag(*runtime, condition.tag_value.empty() ? condition.state_key : condition.tag_value);
    }
    return compareNumber(runtimeNumber(state, condition.object_key, condition.state_key), condition.op, condition.number_value);
}

bool feedbackStateConditionsMatch(const DialogSessionState& state, const DialogFeedbackTemplate& feedback) {
    for (const auto& condition : feedback.state_conditions) {
        if (!stateConditionMatches(state, condition)) return false;
    }
    return true;
}

void syncDerivedRuntimeTags(DialogObjectRuntimeState& runtime) {
    auto sharpness = runtime.numeric_states.find("sharpness");
    if (sharpness == runtime.numeric_states.end()) return;
    if (sharpness->second <= 0.0) {
        sharpness->second = 0.0;
        removeRuntimeTag(runtime, "sharp");
        addRuntimeTag(runtime, "dull");
    } else {
        removeRuntimeTag(runtime, "dull");
        addRuntimeTag(runtime, "sharp");
    }
}

void applyStateMutation(DialogSessionState& state, const DialogScenario& scenario, const DialogStateMutation& mutation) {
    auto& runtime = ensureRuntimeState(state, scenario, mutation.object_key);
    if (mutation.op == "add_number") {
        runtime.numeric_states[mutation.state_key] += mutation.number_value;
    } else if (mutation.op == "set_number") {
        runtime.numeric_states[mutation.state_key] = mutation.number_value;
    } else if (mutation.op == "add_tag") {
        addRuntimeTag(runtime, mutation.tag_value.empty() ? mutation.state_key : mutation.tag_value);
    } else if (mutation.op == "remove_tag") {
        removeRuntimeTag(runtime, mutation.tag_value.empty() ? mutation.state_key : mutation.tag_value);
    }
    syncDerivedRuntimeTags(runtime);
}

void applyStateMutations(DialogSessionState& state, const DialogScenario& scenario, const DialogFeedbackTemplate& feedback) {
    for (const auto& mutation : feedback.state_mutations) {
        applyStateMutation(state, scenario, mutation);
    }
}

Result<DialogFeedbackTemplate> findMatchingFeedback(const DialogScenario& scenario,
                                                    const DialogSessionState& state,
                                                    const std::string& object_key,
                                                    const std::string& target_object_key,
                                                    DialogActionKind action) {
    std::optional<DialogFeedbackTemplate> fallback;
    for (const auto& feedback : scenario.feedbacks) {
        if (feedback.object_key != object_key || feedback.action != action) continue;
        if (!feedback.target_object_key.empty() && !target_object_key.empty() && feedback.target_object_key != target_object_key) continue;
        if (feedback.state_conditions.empty()) {
            if (!fallback.has_value()) fallback = feedback;
            continue;
        }
        if (feedbackStateConditionsMatch(state, feedback)) {
            return Result<DialogFeedbackTemplate>::ok(feedback);
        }
    }
    if (fallback.has_value()) return Result<DialogFeedbackTemplate>::ok(fallback.value());
    return Result<DialogFeedbackTemplate>::fail(makeError(pathfinder::foundation::ErrorCode::id_not_found, "feedback not found"));
}

bool hasAmbiguousDoseWindow(const DialogSessionState& state,
                            const DialogScenario& scenario,
                            const DialogFeedbackTemplate& feedback,
                            uint64_t now_tick) {
    if (feedback.action != DialogActionKind::Eat || feedback.risk_delta <= 0.0) {
        return false;
    }
    const auto family_key = exposureFamilyForObject(scenario, feedback.object_key);
    if (family_key.empty()) return false;

    double recent_dose = 1.0;
    bool has_fresh_like_alternative = false;
    for (const auto& exposure : state.causal_exposures) {
        if (!isRecentExposure(exposure, family_key, "eat", now_tick, kShortExposureWindowTicks)) continue;
        recent_dose += std::max(0.0, exposure.dose_amount);
        if (hasFreshLikeExposure(exposure)) {
            has_fresh_like_alternative = true;
        }
    }
    return recent_dose >= 3.0 && has_fresh_like_alternative;
}

void attachCausalContext(const DialogSessionState& state, const DialogScenario& scenario, DialogFeedbackTemplate& feedback) {
    if (feedback.action == DialogActionKind::Eat) {
        feedback.reason_keys.push_back("causal_route_eat");
    }
    if (hasAmbiguousDoseWindow(state, scenario, feedback, state.current_tick.value())) {
        feedback.reason_keys.push_back("causal_ambiguous_dose_window");
        feedback.warning_keys.push_back("causal_alternative_explanation_not_excluded");
    }
}

void pruneCausalExposureHistory(DialogSessionState& state) {
    if (state.causal_exposures.size() <= kMaxCausalExposureHistory) return;
    state.causal_exposures.erase(
        state.causal_exposures.begin(),
        state.causal_exposures.begin() + static_cast<std::ptrdiff_t>(state.causal_exposures.size() - kMaxCausalExposureHistory));
}

void recordCausalExposure(DialogSessionState& state, const DialogScenario& scenario, const DialogFeedbackTemplate& feedback) {
    const auto family_key = exposureFamilyForObject(scenario, feedback.object_key);
    if (family_key.empty()) return;
    if (feedback.action != DialogActionKind::Eat && feedback.action != DialogActionKind::Use) return;

    DialogCausalExposureRecord exposure;
    exposure.object_key = feedback.object_key;
    exposure.family_key = family_key;
    exposure.action_key = toString(feedback.action);
    exposure.effect_key = feedback.effect_key;
    exposure.observed_tick = state.current_tick.value();
    exposure.dose_amount = feedback.action == DialogActionKind::Eat ? 1.0 : 0.0;
    exposure.state_keys = exposureStatesForObject(scenario, feedback.object_key);
    exposure.reason_keys = feedback.reason_keys;
    state.causal_exposures.push_back(std::move(exposure));
    pruneCausalExposureHistory(state);
}

DialogResponseDto buildWaitResponse(const DialogSessionState& state) {
    DialogResponseDto response;
    response.session_id = state.session_id;
    response.decision = DialogTurnDecision::RepliedOnly;
    response.reply_text = "你等待了一会儿。短时间内的饱腹、消化和近期尝试影响逐渐减弱，但已经形成的记忆和知识不会消失。";
    response.state_summary_lines.push_back("回合：" + std::to_string(state.turn_index));
    response.state_summary_lines.push_back("时间推进：短期暴露窗口已自然衰减");
    return response;
}

} // namespace

DialogTurnService::DialogTurnService()
    : session_store_(std::make_unique<InMemoryDialogSessionStore>()) {
}

Result<DialogResponseDto> DialogTurnService::handle(const DialogRequestDto& request) {
    auto detailed = handleDetailed(request);
    if (detailed.is_error()) return Result<DialogResponseDto>::fail(detailed.errors());
    return Result<DialogResponseDto>::ok(std::move(detailed.value().response));
}

Result<DialogSessionState> DialogTurnService::loadOrCreateSessionSnapshot(const std::string& session_id) {
    return session_store_->loadOrCreate(session_id);
}

Result<void> DialogTurnService::resetSession(const std::string& session_id) {
    return session_store_->reset(session_id);
}

Result<DialogTurnServiceResult> DialogTurnService::handleDetailed(const DialogRequestDto& request) {
    auto wrap = [](const DialogResponseDto& response, const DialogSessionState& state) {
        return Result<DialogTurnServiceResult>::ok(DialogTurnServiceResult{response, state});
    };
    auto rejected = [&](const DialogSessionState& state, const DialogIntent& intent, const std::string& reason_key) {
        auto response = presenter_.buildRejectedResponse(state, intent, reason_key);
        if (response.is_error()) return Result<DialogTurnServiceResult>::fail(response.errors());
        return wrap(response.value(), state);
    };

    auto req_r = request.validateBasic();
    if (req_r.is_error()) {
        DialogTurnServiceResult detailed;
        detailed.response.session_id = request.session_id;
        detailed.response.decision = DialogTurnDecision::Rejected;
        detailed.response.reply_text = "请求格式不正确。";
        return Result<DialogTurnServiceResult>::ok(std::move(detailed));
    }

    auto session_r = session_store_->loadOrCreate(request.session_id);
    if (session_r.is_error()) {
        DialogTurnServiceResult detailed;
        detailed.response.session_id = request.session_id;
        detailed.response.decision = DialogTurnDecision::Failed;
        detailed.response.reply_text = "无法加载会话。";
        return Result<DialogTurnServiceResult>::ok(std::move(detailed));
    }
    auto state = session_r.value();
    state.turn_index++;
    state.current_tick = pathfinder::foundation::Tick(state.current_tick.value() + 1);

    auto intent_r = intent_parser_.parse(request.input_text);
    if (intent_r.is_error()) {
        return rejected(state, DialogIntent{}, "unknown_intent");
    }
    auto intent = intent_r.value();

    if (intent.kind == DialogIntentKind::Unknown) {
        return rejected(state, intent, "unknown_intent");
    }

    if (intent.kind == DialogIntentKind::Observe) {
        auto response = presenter_.buildObserveResponse(state);
        if (response.is_error()) return Result<DialogTurnServiceResult>::fail(response.errors());
        session_store_->save(state);
        return wrap(response.value(), state);
    }

    if (intent.kind == DialogIntentKind::Help) {
        auto response = presenter_.buildHelpResponse(state);
        if (response.is_error()) return Result<DialogTurnServiceResult>::fail(response.errors());
        session_store_->save(state);
        return wrap(response.value(), state);
    }

    if (intent.kind == DialogIntentKind::Wait) {
        state.current_tick = pathfinder::foundation::Tick(state.current_tick.value() + kWaitAdvanceTicks);
        auto response = buildWaitResponse(state);
        session_store_->save(state);
        return wrap(response, state);
    }

    if (intent.kind == DialogIntentKind::Restart) {
        auto reset_r = session_store_->reset(request.session_id);
        if (reset_r.is_error()) {
            DialogResponseDto err;
            err.session_id = request.session_id;
            err.decision = DialogTurnDecision::Failed;
            err.reply_text = "重置失败。";
            return wrap(err, state);
        }
        auto new_state_r = session_store_->loadOrCreate(request.session_id);
        auto new_state = new_state_r.is_ok() ? new_state_r.value() : state;
        auto response = presenter_.buildRestartResponse(new_state);
        if (response.is_error()) return Result<DialogTurnServiceResult>::fail(response.errors());
        return wrap(response.value(), new_state);
    }

    if (intent.kind == DialogIntentKind::InspectActorKnowledge) {
        auto response = presenter_.buildInspectResponse(state, false);
        if (response.is_error()) return Result<DialogTurnServiceResult>::fail(response.errors());
        session_store_->save(state);
        return wrap(response.value(), state);
    }

    if (intent.kind == DialogIntentKind::InspectRecipientKnowledge) {
        auto response = presenter_.buildInspectResponse(state, true);
        if (response.is_error()) return Result<DialogTurnServiceResult>::fail(response.errors());
        session_store_->save(state);
        return wrap(response.value(), state);
    }

    if (intent.kind == DialogIntentKind::RepeatLastAction) {
        if (!state.last_learning_intent.has_value()) {
            return rejected(state, intent, "no_previous_action");
        }
        intent = state.last_learning_intent.value();
    }

    auto scenario_r = scenario_catalog_.defaultScenario();
    if (scenario_r.is_error()) {
        return rejected(state, intent, "scenario_error");
    }
    auto scenario = scenario_r.value();

    if (intent.kind != DialogIntentKind::TeachRecipient) {
        auto obj_r = scenario_catalog_.findObject(scenario, intent.object_key);
        if (obj_r.is_error()) {
            return rejected(state, intent, "object_not_visible");
        }
        auto obj = obj_r.value();
        if (obj->visibility == DialogObjectVisibility::HiddenFromPlayer) {
            return rejected(state, intent, "object_not_visible");
        }
        bool action_allowed = false;
        for (auto action : obj->allowed_actions) {
            if (action == intent.action) {
                action_allowed = true;
                break;
            }
        }
        if (!action_allowed) {
            return rejected(state, intent, "action_not_allowed");
        }
    }

    DialogFeedbackTemplate feedback;
    if (intent.kind == DialogIntentKind::TeachRecipient) {
        const pathfinder::knowledge::KnowledgeClaim* teach_claim = nullptr;
        for (const auto& claim : state.actor_claims) {
            if (claim.status == pathfinder::knowledge::KnowledgeStatus::Active ||
                claim.status == pathfinder::knowledge::KnowledgeStatus::Teachable ||
                claim.status == pathfinder::knowledge::KnowledgeStatus::Hypothesis ||
                claim.status == pathfinder::knowledge::KnowledgeStatus::Candidate) {
                teach_claim = &claim;
                break;
            }
        }
        if (!teach_claim) {
            return rejected(state, intent, "no_teachable_knowledge");
        }
        feedback.feedback_key = "teach_" + state.session_id + "_" + std::to_string(state.turn_index);
        feedback.object_key = teach_claim->subject.subject_id;
        feedback.action = teach_claim->predicate.action_key == "use" ? DialogActionKind::Use : DialogActionKind::Eat;
        feedback.effect_key = teach_claim->predicate.effect_key;
        if (!teach_claim->subject.related_subject_ids.empty()) {
            feedback.target_object_key = teach_claim->subject.related_subject_ids.front();
        }
        feedback.outcome_signals = {pathfinder::cognition::CognitionOutcomeSignal::NoVisibleEffect};
        feedback.utility_delta = 0.0;
        feedback.risk_delta = 0.0;
        feedback.reason_keys.push_back("teach_existing_knowledge");
    } else {
        auto fb_r = findMatchingFeedback(scenario, state, intent.object_key, intent.target_object_key, intent.action);
        if (fb_r.is_error()) {
            return rejected(state, intent, "feedback_not_found");
        }
        feedback = fb_r.value();
        if (intent.target_object_key.empty() && !feedback.target_object_key.empty()) {
            intent.target_object_key = feedback.target_object_key;
        }
        attachCausalContext(state, scenario, feedback);
    }

    auto input_r = learning_adapter_.buildLearningInput(state, intent, feedback);
    if (input_r.is_error()) {
        return rejected(state, intent, "learning_input_failed");
    }
    auto input = input_r.value();

    LearningLoopOptions options;
    options.require_full_chain = false;
    options.route_propagation_conflict_to_revision = true;

    if (intent.kind == DialogIntentKind::TeachRecipient) {
        options.enable_cognition = false;
        options.enable_memory_write = false;
        options.enable_memory_compression = false;
        options.enable_memory_recall = false;
        options.enable_knowledge_formation = false;
        options.enable_knowledge_revision = false;
        options.enable_knowledge_propagation = true;
        options.enable_recipient_projection_check = true;
    } else {
        options.enable_knowledge_propagation = false;
        options.enable_recipient_projection_check = false;
    }

    auto result_r = learning_service_.run(input, options);
    if (result_r.is_error()) {
        return rejected(state, intent, "learning_loop_failed");
    }
    auto result = result_r.value();

    const bool learning_succeeded =
        result.decision == pathfinder::learning::LearningLoopDecision::Completed ||
        result.decision == pathfinder::learning::LearningLoopDecision::PartialCompleted ||
        result.decision == pathfinder::learning::LearningLoopDecision::RoutedToRevision;
    if (!learning_succeeded) {
        return rejected(state, intent, "learning_loop_failed");
    }

    auto report_r = report_builder_.buildReport(input, result, options);
    auto report = report_r.is_ok() ? report_r.value() : LearningDebugReport{};

    auto apply_r = learning_adapter_.applyLearningResult(state, result);
    if (apply_r.is_error()) {
        return rejected(state, intent, "apply_result_failed");
    }

    if (intent.kind != DialogIntentKind::TeachRecipient) {
        applyStateMutations(state, scenario, feedback);
        recordCausalExposure(state, scenario, feedback);
    }

    if (intent.kind != DialogIntentKind::RepeatLastAction &&
        intent.kind != DialogIntentKind::TeachRecipient) {
        state.last_learning_intent = intent;
    }

    auto response = presenter_.buildLearningResponse(state, intent, result, report, feedback.effect_key);
    if (response.is_error()) {
        return rejected(state, intent, "presenter_failed");
    }

    session_store_->save(state);
    return wrap(response.value(), state);
}

} // namespace pathfinder::h5_dialog
