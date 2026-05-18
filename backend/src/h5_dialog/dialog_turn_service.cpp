#include "pathfinder/h5_dialog/dialog_turn_service.h"
#include <algorithm>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::learning::LearningLoopOptions;
using pathfinder::learning::LearningLoopInput;
using pathfinder::learning::LearningLoopResult;
using pathfinder::learning::LearningDebugReport;

DialogTurnService::DialogTurnService()
    : session_store_(std::make_unique<InMemoryDialogSessionStore>()) {
}

Result<DialogResponseDto> DialogTurnService::handle(const DialogRequestDto& request) {
    // 1. Validate request
    auto req_r = request.validateBasic();
    if (req_r.is_error()) {
        DialogResponseDto err;
        err.session_id = request.session_id;
        err.decision = DialogTurnDecision::Rejected;
        err.reply_text = "请求格式不正确。";
        return Result<DialogResponseDto>::ok(err);
    }

    // 2. Load or create session
    auto session_r = session_store_->loadOrCreate(request.session_id);
    if (session_r.is_error()) {
        DialogResponseDto err;
        err.session_id = request.session_id;
        err.decision = DialogTurnDecision::Failed;
        err.reply_text = "无法加载会话。";
        return Result<DialogResponseDto>::ok(err);
    }
    auto state = session_r.value();
    state.turn_index++;
    state.current_tick = pathfinder::foundation::Tick(state.current_tick.value() + 1);

    // 3. Parse intent
    auto intent_r = intent_parser_.parse(request.input_text);
    if (intent_r.is_error()) {
        return presenter_.buildRejectedResponse(state, DialogIntent{}, "unknown_intent");
    }
    auto intent = intent_r.value();

    if (intent.kind == DialogIntentKind::Unknown) {
        return presenter_.buildRejectedResponse(state, intent, "unknown_intent");
    }

    // 4. Handle non-learning intents
    if (intent.kind == DialogIntentKind::Observe) {
        auto resp_r = presenter_.buildObserveResponse(state);
        if (resp_r.is_ok()) {
            session_store_->save(state);
        }
        return resp_r;
    }

    if (intent.kind == DialogIntentKind::Help) {
        auto resp_r = presenter_.buildHelpResponse(state);
        if (resp_r.is_ok()) {
            session_store_->save(state);
        }
        return resp_r;
    }

    if (intent.kind == DialogIntentKind::Restart) {
        auto reset_r = session_store_->reset(request.session_id);
        if (reset_r.is_error()) {
            DialogResponseDto err;
            err.session_id = request.session_id;
            err.decision = DialogTurnDecision::Failed;
            err.reply_text = "重置失败。";
            return Result<DialogResponseDto>::ok(err);
        }
        auto new_state_r = session_store_->loadOrCreate(request.session_id);
        if (new_state_r.is_ok()) {
            return presenter_.buildRestartResponse(new_state_r.value());
        }
        return presenter_.buildRestartResponse(state);
    }

    if (intent.kind == DialogIntentKind::InspectActorKnowledge) {
        auto resp_r = presenter_.buildInspectResponse(state, false);
        if (resp_r.is_ok()) {
            session_store_->save(state);
        }
        return resp_r;
    }

    if (intent.kind == DialogIntentKind::InspectRecipientKnowledge) {
        auto resp_r = presenter_.buildInspectResponse(state, true);
        if (resp_r.is_ok()) {
            session_store_->save(state);
        }
        return resp_r;
    }

    // 5. Handle RepeatLastAction
    if (intent.kind == DialogIntentKind::RepeatLastAction) {
        if (!state.last_learning_intent.has_value()) {
            return presenter_.buildRejectedResponse(state, intent, "no_previous_action");
        }
        intent = state.last_learning_intent.value();
    }

    // 6. Load scenario and validate object/action
    auto scenario_r = scenario_catalog_.defaultScenario();
    if (scenario_r.is_error()) {
        return presenter_.buildRejectedResponse(state, intent, "scenario_error");
    }
    auto scenario = scenario_r.value();

    if (intent.kind == DialogIntentKind::TeachRecipient) {
        // Teaching does not need an object from scenario
    } else {
        // Check object exists and is visible
        auto obj_r = scenario_catalog_.findObject(scenario, intent.object_key);
        if (obj_r.is_error()) {
            return presenter_.buildRejectedResponse(state, intent, "object_not_visible");
        }
        auto obj = obj_r.value();
        if (obj->visibility == DialogObjectVisibility::HiddenFromPlayer) {
            return presenter_.buildRejectedResponse(state, intent, "object_not_visible");
        }
        // Check action allowed
        bool action_allowed = false;
        for (auto a : obj->allowed_actions) {
            if (a == intent.action) {
                action_allowed = true;
                break;
            }
        }
        if (!action_allowed) {
            return presenter_.buildRejectedResponse(state, intent, "action_not_allowed");
        }
    }

    // 7. Find feedback template (for TryEat/TryUse) or build from claims (for Teach)
    DialogFeedbackTemplate feedback;
    if (intent.kind == DialogIntentKind::TeachRecipient) {
        const pathfinder::knowledge::KnowledgeClaim* teach_claim = nullptr;
        for (const auto& claim : state.actor_claims) {
            if (claim.status == pathfinder::knowledge::KnowledgeStatus::Active ||
                claim.status == pathfinder::knowledge::KnowledgeStatus::Teachable ||
                claim.status == pathfinder::knowledge::KnowledgeStatus::Hypothesis) {
                teach_claim = &claim;
                break;
            }
        }
        if (!teach_claim) {
            return presenter_.buildRejectedResponse(state, intent, "no_teachable_knowledge");
        }
        feedback.feedback_key = "teach_" + state.session_id + "_" + std::to_string(state.turn_index);
        feedback.object_key = teach_claim->subject.subject_id;
        feedback.action = teach_claim->predicate.action_key == "use" ? DialogActionKind::Use : DialogActionKind::Eat;
        feedback.effect_key = teach_claim->predicate.effect_key;
        feedback.outcome_signals = {pathfinder::cognition::CognitionOutcomeSignal::NoVisibleEffect};
        feedback.utility_delta = 0.0;
        feedback.risk_delta = 0.0;
        feedback.reason_keys.push_back("teach_existing_knowledge");
    } else {
        auto fb_r = scenario_catalog_.findFeedback(scenario, intent.object_key, intent.action);
        if (fb_r.is_error()) {
            return presenter_.buildRejectedResponse(state, intent, "feedback_not_found");
        }
        feedback = fb_r.value();
    }

    // 8. Build learning input
    auto input_r = learning_adapter_.buildLearningInput(state, intent, feedback);
    if (input_r.is_error()) {
        return presenter_.buildRejectedResponse(state, intent, "learning_input_failed");
    }
    auto input = input_r.value();

    // 9. Run learning loop
    LearningLoopOptions options;
    options.require_full_chain = false;
    options.route_propagation_conflict_to_revision = true;

    if (intent.kind == DialogIntentKind::TeachRecipient) {
        // Teaching: skip cognition/memory/formation, only propagate existing claims.
        options.enable_cognition = false;
        options.enable_memory_write = false;
        options.enable_memory_compression = false;
        options.enable_memory_recall = false;
        options.enable_knowledge_formation = false;
        options.enable_knowledge_revision = false;
        options.enable_knowledge_propagation = true;
        options.enable_recipient_projection_check = true;
    } else {
        // Discovery: only actor learns, do not auto-propagate to recipient.
        options.enable_knowledge_propagation = false;
        options.enable_recipient_projection_check = false;
    }

    auto result_r = learning_service_.run(input, options);
    if (result_r.is_error()) {
        return presenter_.buildRejectedResponse(state, intent, "learning_loop_failed");
    }
    auto result = result_r.value();

    const bool learning_succeeded =
        result.decision == pathfinder::learning::LearningLoopDecision::Completed ||
        result.decision == pathfinder::learning::LearningLoopDecision::PartialCompleted ||
        result.decision == pathfinder::learning::LearningLoopDecision::RoutedToRevision;
    if (!learning_succeeded) {
        return presenter_.buildRejectedResponse(state, intent, "learning_loop_failed");
    }

    // 10. Build debug report
    auto report_r = report_builder_.buildReport(input, result, options);
    if (report_r.is_error()) {
        // Non-fatal: continue without report
    }
    auto report = report_r.is_ok() ? report_r.value() : LearningDebugReport{};

    // 11. Apply result to session
    auto apply_r = learning_adapter_.applyLearningResult(state, result);
    if (apply_r.is_error()) {
        return presenter_.buildRejectedResponse(state, intent, "apply_result_failed");
    }

    // 12. Record last learning intent
    if (intent.kind != DialogIntentKind::RepeatLastAction &&
        intent.kind != DialogIntentKind::TeachRecipient) {
        state.last_learning_intent = intent;
    }

    // 13. Build response
    auto resp_r = presenter_.buildLearningResponse(state, intent, result, report);
    if (resp_r.is_error()) {
        return presenter_.buildRejectedResponse(state, intent, "presenter_failed");
    }

    // 14. Save session
    session_store_->save(state);

    return resp_r;
}

} // namespace pathfinder::h5_dialog
