#include "pathfinder/reaction/reaction_trace.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::reaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

Result<void> ReactionTrace::validateBasic() const {
    if (decision == ReactionDecision::Unknown || decision == ReactionDecision::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionTrace decision invalid"));
    }
    if (containsReactionForbiddenKey(input_key) || containsReactionForbiddenKey(selected_rule_key) ||
        containsReactionForbiddenKey(matched_rule_keys) || containsReactionForbiddenKey(blocked_rule_keys) ||
        containsReactionForbiddenKey(no_match_reason_keys) || containsReactionForbiddenKey(output_summary_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionTrace contains forbidden key"));
    }
    for (const auto& trace : condition_traces) {
        auto valid = trace.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

} // namespace pathfinder::reaction
