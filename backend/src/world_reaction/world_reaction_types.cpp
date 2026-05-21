#include "pathfinder/world_reaction/world_reaction_types.h"

namespace pathfinder::world_reaction {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

// ---------------------------------------------------------------------------
// WorldReactionActionKind
// ---------------------------------------------------------------------------

std::string toString(WorldReactionActionKind kind) {
    switch (kind) {
        case WorldReactionActionKind::Craft:   return "craft";
        case WorldReactionActionKind::Use:     return "use";
        case WorldReactionActionKind::Combine: return "combine";
        default: return "unknown";
    }
}

Result<WorldReactionActionKind> worldReactionActionKindFromString(const std::string& str) {
    if (str == "craft")   return Result<WorldReactionActionKind>::ok(WorldReactionActionKind::Craft);
    if (str == "use")     return Result<WorldReactionActionKind>::ok(WorldReactionActionKind::Use);
    if (str == "combine") return Result<WorldReactionActionKind>::ok(WorldReactionActionKind::Combine);
    if (str == "unknown") return Result<WorldReactionActionKind>::ok(WorldReactionActionKind::Unknown);
    return Result<WorldReactionActionKind>::fail(
        makeError(ErrorCode::command_invalid_argument, "invalid_action_kind",
                  "Unknown reaction action kind: " + str));
}

// ---------------------------------------------------------------------------
// WorldReactionInputSourceKind
// ---------------------------------------------------------------------------

std::string toString(WorldReactionInputSourceKind kind) {
    switch (kind) {
        case WorldReactionInputSourceKind::ActorInventory: return "actor_inventory";
        case WorldReactionInputSourceKind::SameCellMap:    return "same_cell_map";
        case WorldReactionInputSourceKind::AdjacentMap:    return "adjacent_map";
        default: return "unknown";
    }
}

// ---------------------------------------------------------------------------
// WorldReactionOutputLocationPolicy
// ---------------------------------------------------------------------------

std::string toString(WorldReactionOutputLocationPolicy policy) {
    switch (policy) {
        case WorldReactionOutputLocationPolicy::ActorInventory: return "actor_inventory";
        case WorldReactionOutputLocationPolicy::DropOnMap:      return "drop_on_map";
        default: return "unknown";
    }
}

Result<WorldReactionOutputLocationPolicy> worldReactionOutputLocationPolicyFromString(const std::string& str) {
    if (str == "actor_inventory") return Result<WorldReactionOutputLocationPolicy>::ok(WorldReactionOutputLocationPolicy::ActorInventory);
    if (str == "drop_on_map")     return Result<WorldReactionOutputLocationPolicy>::ok(WorldReactionOutputLocationPolicy::DropOnMap);
    if (str == "unknown")         return Result<WorldReactionOutputLocationPolicy>::ok(WorldReactionOutputLocationPolicy::Unknown);
    return Result<WorldReactionOutputLocationPolicy>::fail(
        makeError(ErrorCode::command_invalid_argument, "invalid_output_policy",
                  "Unknown output location policy: " + str));
}

// ---------------------------------------------------------------------------
// WorldReactionFailureKind
// ---------------------------------------------------------------------------

std::string toString(WorldReactionFailureKind kind) {
    switch (kind) {
        case WorldReactionFailureKind::InvalidRequest:        return "invalid_request";
        case WorldReactionFailureKind::ActorMissing:          return "actor_missing";
        case WorldReactionFailureKind::ReactionMissing:       return "reaction_missing";
        case WorldReactionFailureKind::ReactionInvalid:       return "reaction_invalid";
        case WorldReactionFailureKind::ActionMismatch:        return "action_mismatch";
        case WorldReactionFailureKind::InventoryMissing:      return "inventory_missing";
        case WorldReactionFailureKind::InputMissing:          return "input_missing";
        case WorldReactionFailureKind::InputStateMismatch:    return "input_state_mismatch";
        case WorldReactionFailureKind::QuantityInsufficient:  return "quantity_insufficient";
        case WorldReactionFailureKind::OutputInvalid:         return "output_invalid";
        case WorldReactionFailureKind::OutputPolicyInvalid:   return "output_policy_invalid";
        case WorldReactionFailureKind::ConsumeFailed:         return "consume_failed";
        case WorldReactionFailureKind::SpawnFailed:           return "spawn_failed";
        case WorldReactionFailureKind::RuntimeConflict:       return "runtime_conflict";
        default: return "none";
    }
}

// ---------------------------------------------------------------------------
// WorldReactionRequest
// ---------------------------------------------------------------------------

Result<void> WorldReactionRequest::validateBasic() const {
    if (reaction_command_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_invalid_argument, "missing_command_id",
                                            "reaction_command_id is required"));
    }
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_invalid_argument, "missing_actor_key",
                                            "actor_key is required"));
    }
    if (reaction_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_invalid_argument, "missing_reaction_key",
                                            "reaction_key is required"));
    }
    if (action_kind == WorldReactionActionKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::command_invalid_argument, "unknown_action_kind",
                                            "action_kind cannot be Unknown"));
    }
    if (output_location_policy == WorldReactionOutputLocationPolicy::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::command_invalid_argument, "unknown_output_policy",
                                            "output_location_policy cannot be Unknown"));
    }
    return Result<void>::ok();
}

} // namespace pathfinder::world_reaction
