// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/h5_projection/projection.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::h5_playable {

enum class PlayableInputKind {
    Unknown,
    FreeText,
    ProjectionAction,
    Reset,
    Bootstrap,
    TestOnly
};

enum class PlayableFeedbackTone {
    Unknown,
    Neutral,
    Positive,
    Warning,
    Danger,
    System,
    TestOnly
};

std::string toString(PlayableInputKind value);
std::string toString(PlayableFeedbackTone value);
pathfinder::foundation::Result<PlayableInputKind> playableInputKindFromString(const std::string& value);
pathfinder::foundation::Result<PlayableFeedbackTone> playableFeedbackToneFromString(const std::string& value);

bool containsPlayableForbiddenKey(const std::string& value);
bool containsPlayableForbiddenKey(const std::vector<std::string>& values);

struct H5PlayableBootstrapRequest {
    std::string session_id;
    bool reset{false};
    std::string language{"zh_cn"};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct H5PlayableRequest {
    std::string session_id;
    uint64_t client_turn{0};
    PlayableInputKind input_kind{PlayableInputKind::Unknown};
    std::string input_text;
    std::optional<std::string> action_key;
    std::optional<std::string> target_object_ref;
    bool request_projection{true};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct H5PlayableExecutionStatus {
    bool visible{false};
    std::optional<pathfinder::h5_projection::SafeTextProjection> current_goal;
    std::optional<pathfinder::h5_projection::SafeTextProjection> active_step;
    std::optional<pathfinder::h5_projection::SafeTextProjection> blocked_by;
    std::optional<pathfinder::h5_projection::SafeTextProjection> interrupt_reason;
    std::optional<pathfinder::h5_projection::SafeTextProjection> response_plan;
    std::optional<pathfinder::h5_projection::SafeTextProjection> resume_hint;
    std::vector<pathfinder::h5_projection::SafeTextProjection> trace_lines;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct H5PlayableResponse {
    bool ok{false};
    std::string session_id;
    uint64_t server_turn{0};
    PlayableFeedbackTone tone{PlayableFeedbackTone::Neutral};
    pathfinder::h5_projection::SafeTextProjection reply_text;
    pathfinder::h5_projection::H5GameProjection projection;
    std::optional<H5PlayableExecutionStatus> execution_status;
    std::vector<pathfinder::h5_projection::ProjectionIssue> issues;
    std::vector<std::string> debug_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::h5_playable
