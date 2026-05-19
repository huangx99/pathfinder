#include "pathfinder/h5_dialog/dialog_types.h"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// Enum toString / fromString
// ============================================================

std::string toString(DialogIntentKind kind) {
    switch (kind) {
        case DialogIntentKind::Unknown: return "unknown";
        case DialogIntentKind::Observe: return "observe";
        case DialogIntentKind::TryEat: return "try_eat";
        case DialogIntentKind::TryUse: return "try_use";
        case DialogIntentKind::RepeatLastAction: return "repeat_last_action";
        case DialogIntentKind::TeachRecipient: return "teach_recipient";
        case DialogIntentKind::InspectActorKnowledge: return "inspect_actor_knowledge";
        case DialogIntentKind::InspectRecipientKnowledge: return "inspect_recipient_knowledge";
        case DialogIntentKind::Help: return "help";
        case DialogIntentKind::Wait: return "wait";
        case DialogIntentKind::Restart: return "restart";
        case DialogIntentKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<DialogIntentKind> dialogIntentKindFromString(const std::string& str) {
    if (str == "unknown") return Result<DialogIntentKind>::ok(DialogIntentKind::Unknown);
    if (str == "observe") return Result<DialogIntentKind>::ok(DialogIntentKind::Observe);
    if (str == "try_eat") return Result<DialogIntentKind>::ok(DialogIntentKind::TryEat);
    if (str == "try_use") return Result<DialogIntentKind>::ok(DialogIntentKind::TryUse);
    if (str == "repeat_last_action") return Result<DialogIntentKind>::ok(DialogIntentKind::RepeatLastAction);
    if (str == "teach_recipient") return Result<DialogIntentKind>::ok(DialogIntentKind::TeachRecipient);
    if (str == "inspect_actor_knowledge") return Result<DialogIntentKind>::ok(DialogIntentKind::InspectActorKnowledge);
    if (str == "inspect_recipient_knowledge") return Result<DialogIntentKind>::ok(DialogIntentKind::InspectRecipientKnowledge);
    if (str == "help") return Result<DialogIntentKind>::ok(DialogIntentKind::Help);
    if (str == "wait") return Result<DialogIntentKind>::ok(DialogIntentKind::Wait);
    if (str == "restart") return Result<DialogIntentKind>::ok(DialogIntentKind::Restart);
    if (str == "test_only") return Result<DialogIntentKind>::ok(DialogIntentKind::TestOnly);
    return Result<DialogIntentKind>::fail(makeError(ErrorCode::validation_failed, "Unknown DialogIntentKind string"));
}

std::string toString(DialogActionKind kind) {
    switch (kind) {
        case DialogActionKind::Unknown: return "unknown";
        case DialogActionKind::Observe: return "observe";
        case DialogActionKind::Eat: return "eat";
        case DialogActionKind::Use: return "use";
        case DialogActionKind::Teach: return "teach";
        case DialogActionKind::Inspect: return "inspect";
        case DialogActionKind::Wait: return "wait";
        case DialogActionKind::Restart: return "restart";
        case DialogActionKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<DialogActionKind> dialogActionKindFromString(const std::string& str) {
    if (str == "unknown") return Result<DialogActionKind>::ok(DialogActionKind::Unknown);
    if (str == "observe") return Result<DialogActionKind>::ok(DialogActionKind::Observe);
    if (str == "eat") return Result<DialogActionKind>::ok(DialogActionKind::Eat);
    if (str == "use") return Result<DialogActionKind>::ok(DialogActionKind::Use);
    if (str == "teach") return Result<DialogActionKind>::ok(DialogActionKind::Teach);
    if (str == "inspect") return Result<DialogActionKind>::ok(DialogActionKind::Inspect);
    if (str == "wait") return Result<DialogActionKind>::ok(DialogActionKind::Wait);
    if (str == "restart") return Result<DialogActionKind>::ok(DialogActionKind::Restart);
    if (str == "test_only") return Result<DialogActionKind>::ok(DialogActionKind::TestOnly);
    return Result<DialogActionKind>::fail(makeError(ErrorCode::validation_failed, "Unknown DialogActionKind string"));
}

std::string toString(DialogTurnDecision decision) {
    switch (decision) {
        case DialogTurnDecision::Unknown: return "unknown";
        case DialogTurnDecision::RepliedOnly: return "replied_only";
        case DialogTurnDecision::LearningLoopRan: return "learning_loop_ran";
        case DialogTurnDecision::TeachingRan: return "teaching_ran";
        case DialogTurnDecision::Rejected: return "rejected";
        case DialogTurnDecision::Failed: return "failed";
        case DialogTurnDecision::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<DialogTurnDecision> dialogTurnDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<DialogTurnDecision>::ok(DialogTurnDecision::Unknown);
    if (str == "replied_only") return Result<DialogTurnDecision>::ok(DialogTurnDecision::RepliedOnly);
    if (str == "learning_loop_ran") return Result<DialogTurnDecision>::ok(DialogTurnDecision::LearningLoopRan);
    if (str == "teaching_ran") return Result<DialogTurnDecision>::ok(DialogTurnDecision::TeachingRan);
    if (str == "rejected") return Result<DialogTurnDecision>::ok(DialogTurnDecision::Rejected);
    if (str == "failed") return Result<DialogTurnDecision>::ok(DialogTurnDecision::Failed);
    if (str == "test_only") return Result<DialogTurnDecision>::ok(DialogTurnDecision::TestOnly);
    return Result<DialogTurnDecision>::fail(makeError(ErrorCode::validation_failed, "Unknown DialogTurnDecision string"));
}

std::string toString(DialogObjectVisibility visibility) {
    switch (visibility) {
        case DialogObjectVisibility::Unknown: return "unknown";
        case DialogObjectVisibility::Visible: return "visible";
        case DialogObjectVisibility::Mentioned: return "mentioned";
        case DialogObjectVisibility::HiddenFromPlayer: return "hidden_from_player";
        case DialogObjectVisibility::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<DialogObjectVisibility> dialogObjectVisibilityFromString(const std::string& str) {
    if (str == "unknown") return Result<DialogObjectVisibility>::ok(DialogObjectVisibility::Unknown);
    if (str == "visible") return Result<DialogObjectVisibility>::ok(DialogObjectVisibility::Visible);
    if (str == "mentioned") return Result<DialogObjectVisibility>::ok(DialogObjectVisibility::Mentioned);
    if (str == "hidden_from_player") return Result<DialogObjectVisibility>::ok(DialogObjectVisibility::HiddenFromPlayer);
    if (str == "test_only") return Result<DialogObjectVisibility>::ok(DialogObjectVisibility::TestOnly);
    return Result<DialogObjectVisibility>::fail(makeError(ErrorCode::validation_failed, "Unknown DialogObjectVisibility string"));
}

// ============================================================
// Security Guard
// ============================================================

std::vector<std::string> dialogForbiddenKeys() {
    return {
        "hidden_truth",
        "real_effect",
        "true_trait",
        "object_truth",
        "raw_state",
        "edible_profile",
        "hunger_delta",
        "health_delta",
        "effect_kind",
        "GameState",
        "AgentTickRecord",
        "SaveGame",
        "SaveManager",
        "AgentRuntime",
        "Policy",
        "RulePipeline"
    };
}

bool containsDialogForbiddenKey(const std::string& text) {
    if (text.empty()) return false;
    for (const auto& key : dialogForbiddenKeys()) {
        if (text.find(key) != std::string::npos) return true;
    }
    return false;
}

bool containsDialogForbiddenKey(const std::vector<std::string>& values) {
    for (const auto& v : values) {
        if (containsDialogForbiddenKey(v)) return true;
    }
    return false;
}

bool containsDialogInjectionKey(const std::string& text) {
    auto lowered = text;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowered.find("<script") != std::string::npos ||
           lowered.find("</script") != std::string::npos ||
           lowered.find("javascript:") != std::string::npos ||
           lowered.find("onerror=") != std::string::npos ||
           lowered.find("onload=") != std::string::npos;
}

// ============================================================
// DTO validateBasic
// ============================================================

Result<void> DialogRequestDto::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "session_id empty"));
    }
    if (input_text.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "input_text empty"));
    }
    if (input_text.size() > 256) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "input_text too long"));
    }
    if (containsDialogForbiddenKey(input_text)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "input_text contains forbidden key"));
    }
    if (containsDialogInjectionKey(input_text)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "input_text contains injection key"));
    }
    if (containsDialogForbiddenKey(session_id)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "session_id contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> DialogIntent::validateBasic() const {
    if (kind == DialogIntentKind::Unknown || kind == DialogIntentKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "invalid intent kind"));
    }
    if (action == DialogActionKind::Unknown || action == DialogActionKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "invalid action kind"));
    }
    if (containsDialogForbiddenKey(object_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "object_key contains forbidden key"));
    }
    if (containsDialogForbiddenKey(target_object_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "target_object_key contains forbidden key"));
    }
    if (containsDialogForbiddenKey(recipient_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "recipient_key contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> DialogQuickActionDto::validateBasic() const {
    if (label_text.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "label_text empty"));
    }
    if (input_text.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "input_text empty"));
    }
    if (containsDialogForbiddenKey(label_text) || containsDialogForbiddenKey(input_text)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "quick action contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> DialogResponseDto::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "session_id empty"));
    }
    if (decision == DialogTurnDecision::Unknown || decision == DialogTurnDecision::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "invalid turn decision"));
    }
    if (containsDialogForbiddenKey(reply_text)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "reply_text contains forbidden key"));
    }
    if (containsDialogForbiddenKey(state_summary_lines)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "state_summary_lines contains forbidden key"));
    }
    if (containsDialogForbiddenKey(actor_knowledge_lines)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "actor_knowledge_lines contains forbidden key"));
    }
    if (containsDialogForbiddenKey(recipient_knowledge_lines)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "recipient_knowledge_lines contains forbidden key"));
    }
    for (const auto& qa : quick_actions) {
        auto r = qa.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

Result<void> DialogScenarioObject::validateBasic() const {
    if (object_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "object_key empty"));
    }
    if (display_name.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "display_name empty"));
    }
    if (containsDialogForbiddenKey(object_key) || containsDialogForbiddenKey(display_name) || containsDialogForbiddenKey(player_description)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "scenario object contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> DialogStateCondition::validateBasic() const {
    if (object_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "state condition object_key empty"));
    }
    if (state_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "state condition state_key empty"));
    }
    if (op.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "state condition op empty"));
    }
    if (containsDialogForbiddenKey(object_key) || containsDialogForbiddenKey(state_key) || containsDialogForbiddenKey(op) || containsDialogForbiddenKey(tag_value)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "state condition contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> DialogStateMutation::validateBasic() const {
    if (object_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "state mutation object_key empty"));
    }
    if (state_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "state mutation state_key empty"));
    }
    if (op.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "state mutation op empty"));
    }
    if (containsDialogForbiddenKey(object_key) || containsDialogForbiddenKey(state_key) || containsDialogForbiddenKey(op) || containsDialogForbiddenKey(tag_value)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "state mutation contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> DialogFeedbackTemplate::validateBasic() const {
    if (feedback_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "feedback_key empty"));
    }
    if (object_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "object_key empty"));
    }
    if (effect_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "effect_key empty"));
    }
    if (action == DialogActionKind::Unknown || action == DialogActionKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "invalid action kind"));
    }
    if (containsDialogForbiddenKey(feedback_key) || containsDialogForbiddenKey(object_key) || containsDialogForbiddenKey(target_object_key) || containsDialogForbiddenKey(effect_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "feedback contains forbidden key"));
    }
    for (const auto& condition : state_conditions) {
        auto result = condition.validateBasic();
        if (result.is_error()) return result;
    }
    for (const auto& mutation : state_mutations) {
        auto result = mutation.validateBasic();
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> DialogObjectRuntimeState::validateBasic() const {
    if (object_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "runtime object_key empty"));
    }
    if (containsDialogForbiddenKey(object_key) || containsDialogForbiddenKey(tag_states)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "runtime state contains forbidden key"));
    }
    for (const auto& pair : numeric_states) {
        if (containsDialogForbiddenKey(pair.first)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "runtime numeric key contains forbidden key"));
        }
    }
    return Result<void>::ok();
}

Result<void> DialogSessionState::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "session_id empty"));
    }
    if (scenario_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "scenario_key empty"));
    }
    auto actor_r = actor.validateBasic();
    if (actor_r.is_error()) return actor_r;
    auto recip_r = recipient.validateBasic();
    if (recip_r.is_error()) return recip_r;
    if (containsDialogForbiddenKey(session_id) || containsDialogForbiddenKey(scenario_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "session contains forbidden key"));
    }
    return Result<void>::ok();
}

} // namespace pathfinder::h5_dialog
