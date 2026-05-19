#include "pathfinder/h5_playable/playable_types.h"
#include <algorithm>
#include <cctype>

namespace pathfinder::h5_playable {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace {

Result<void> fail(const std::string& key) {
    return Result<void>::fail(makeError(ErrorCode::validation_failed, key));
}

template <typename T>
Result<T> failEnum(const std::string& key) {
    return Result<T>::fail(makeError(ErrorCode::validation_enum_unknown, key));
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

Result<void> validateSafeValue(const std::string& value, const std::string& key, bool required = true) {
    if (required && value.empty()) return fail(key + ".missing");
    if (!value.empty() && containsPlayableForbiddenKey(value)) return fail(key + ".forbidden");
    return Result<void>::ok();
}

} // namespace

std::string toString(PlayableInputKind value) {
    switch (value) {
        case PlayableInputKind::Unknown: return "unknown";
        case PlayableInputKind::FreeText: return "free_text";
        case PlayableInputKind::ProjectionAction: return "projection_action";
        case PlayableInputKind::Reset: return "reset";
        case PlayableInputKind::Bootstrap: return "bootstrap";
        case PlayableInputKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(PlayableFeedbackTone value) {
    switch (value) {
        case PlayableFeedbackTone::Unknown: return "unknown";
        case PlayableFeedbackTone::Neutral: return "neutral";
        case PlayableFeedbackTone::Positive: return "positive";
        case PlayableFeedbackTone::Warning: return "warning";
        case PlayableFeedbackTone::Danger: return "danger";
        case PlayableFeedbackTone::System: return "system";
        case PlayableFeedbackTone::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<PlayableInputKind> playableInputKindFromString(const std::string& value) {
    auto normalized = lower(value);
    if (normalized == "free_text") return Result<PlayableInputKind>::ok(PlayableInputKind::FreeText);
    if (normalized == "projection_action") return Result<PlayableInputKind>::ok(PlayableInputKind::ProjectionAction);
    if (normalized == "reset") return Result<PlayableInputKind>::ok(PlayableInputKind::Reset);
    if (normalized == "bootstrap") return Result<PlayableInputKind>::ok(PlayableInputKind::Bootstrap);
    if (normalized == "test_only") return Result<PlayableInputKind>::ok(PlayableInputKind::TestOnly);
    if (normalized == "unknown") return Result<PlayableInputKind>::ok(PlayableInputKind::Unknown);
    return failEnum<PlayableInputKind>("h5_playable.input_kind.unknown");
}

Result<PlayableFeedbackTone> playableFeedbackToneFromString(const std::string& value) {
    auto normalized = lower(value);
    if (normalized == "neutral") return Result<PlayableFeedbackTone>::ok(PlayableFeedbackTone::Neutral);
    if (normalized == "positive") return Result<PlayableFeedbackTone>::ok(PlayableFeedbackTone::Positive);
    if (normalized == "warning") return Result<PlayableFeedbackTone>::ok(PlayableFeedbackTone::Warning);
    if (normalized == "danger") return Result<PlayableFeedbackTone>::ok(PlayableFeedbackTone::Danger);
    if (normalized == "system") return Result<PlayableFeedbackTone>::ok(PlayableFeedbackTone::System);
    if (normalized == "test_only") return Result<PlayableFeedbackTone>::ok(PlayableFeedbackTone::TestOnly);
    if (normalized == "unknown") return Result<PlayableFeedbackTone>::ok(PlayableFeedbackTone::Unknown);
    return failEnum<PlayableFeedbackTone>("h5_playable.feedback_tone.unknown");
}

bool containsPlayableForbiddenKey(const std::string& value) {
    if (value.empty()) return false;
    if (pathfinder::h5_projection::containsProjectionForbiddenKey(value)) return true;
    const auto lowered = lower(value);
    static const std::vector<std::string> forbidden = {
        "edible_profile", "hunger_delta", "health_delta", "object_truth", "debug_summary"
    };
    for (const auto& key : forbidden) {
        if (lowered.find(key) != std::string::npos) return true;
    }
    return false;
}

bool containsPlayableForbiddenKey(const std::vector<std::string>& values) {
    for (const auto& value : values) {
        if (containsPlayableForbiddenKey(value)) return true;
    }
    return false;
}

Result<void> H5PlayableBootstrapRequest::validateBasic() const {
    auto session_result = validateSafeValue(session_id, "h5_playable.bootstrap.session_id");
    if (session_result.is_error()) return session_result;
    if (language != "zh_cn") return fail("h5_playable.bootstrap.language_unsupported");
    return Result<void>::ok();
}

Result<void> H5PlayableRequest::validateBasic() const {
    auto session_result = validateSafeValue(session_id, "h5_playable.turn.session_id");
    if (session_result.is_error()) return session_result;
    if (input_kind == PlayableInputKind::Unknown || input_kind == PlayableInputKind::TestOnly) return fail("h5_playable.turn.input_kind_invalid");
    if (input_text.empty() && !action_key.has_value()) return fail("h5_playable.turn.empty_input");
    if (input_text.size() > 128) return fail("h5_playable.turn.input_too_long");
    auto input_result = validateSafeValue(input_text, "h5_playable.turn.input_text", false);
    if (input_result.is_error()) return input_result;
    if (action_key.has_value()) {
        auto action_result = validateSafeValue(*action_key, "h5_playable.turn.action_key");
        if (action_result.is_error()) return action_result;
    }
    if (target_object_ref.has_value()) {
        auto target_result = validateSafeValue(*target_object_ref, "h5_playable.turn.target_object_ref");
        if (target_result.is_error()) return target_result;
    }
    return Result<void>::ok();
}

Result<void> H5PlayableExecutionStatus::validateBasic() const {
    auto validate_optional = [](const std::optional<pathfinder::h5_projection::SafeTextProjection>& text) {
        if (!text.has_value()) return Result<void>::ok();
        return text->validateBasic();
    };
    for (const auto* text : {&current_goal, &active_step, &blocked_by, &interrupt_reason, &response_plan, &resume_hint}) {
        auto result = validate_optional(*text);
        if (result.is_error()) return result;
    }
    for (const auto& line : trace_lines) {
        auto result = line.validateBasic();
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> H5PlayableResponse::validateBasic() const {
    auto session_result = validateSafeValue(session_id, "h5_playable.response.session_id");
    if (session_result.is_error()) return session_result;
    if (tone == PlayableFeedbackTone::Unknown || tone == PlayableFeedbackTone::TestOnly) return fail("h5_playable.response.tone_invalid");
    auto reply_result = reply_text.validateBasic();
    if (reply_result.is_error()) return reply_result;
    if (reply_text.kind == pathfinder::h5_projection::SafeTextKind::DebugSummary) return fail("h5_playable.response.reply_debug_forbidden");
    auto projection_result = projection.validateBasic();
    if (projection_result.is_error()) return projection_result;
    if (execution_status.has_value()) {
        auto execution_result = execution_status->validateBasic();
        if (execution_result.is_error()) return execution_result;
    }
    for (const auto& issue : issues) {
        auto issue_result = issue.validateBasic();
        if (issue_result.is_error()) return issue_result;
    }
    if (containsPlayableForbiddenKey(debug_keys)) return fail("h5_playable.response.debug_forbidden");
    return Result<void>::ok();
}

} // namespace pathfinder::h5_playable
