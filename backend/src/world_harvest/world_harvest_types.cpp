#include "pathfinder/world_harvest/world_harvest_types.h"

#include "pathfinder/foundation/result.h"

namespace pathfinder::world_harvest {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// toString helpers
// ---------------------------------------------------------------------------

std::string toString(ResourceHarvestKind kind) {
    switch (kind) {
        case ResourceHarvestKind::Unknown: return "unknown";
        case ResourceHarvestKind::Gather:  return "gather";
        case ResourceHarvestKind::Chop:    return "chop";
        case ResourceHarvestKind::Mine:    return "mine";
        case ResourceHarvestKind::Dig:     return "dig";
    }
    return "unknown";
}

ResourceHarvestKind harvestKindFromCommandKind(world_command::WorldCommandKind kind) {
    switch (kind) {
        case world_command::WorldCommandKind::Gather: return ResourceHarvestKind::Gather;
        case world_command::WorldCommandKind::Chop:   return ResourceHarvestKind::Chop;
        case world_command::WorldCommandKind::Mine:   return ResourceHarvestKind::Mine;
        case world_command::WorldCommandKind::Dig:    return ResourceHarvestKind::Dig;
        default: return ResourceHarvestKind::Unknown;
    }
}

std::string toString(ResourceHarvestFailureKind kind) {
    switch (kind) {
        case ResourceHarvestFailureKind::None:             return "none";
        case ResourceHarvestFailureKind::InvalidRequest:   return "invalid_request";
        case ResourceHarvestFailureKind::ActorMissing:     return "actor_missing";
        case ResourceHarvestFailureKind::NodeMissing:      return "node_missing";
        case ResourceHarvestFailureKind::NodeNotVisible:   return "node_not_visible";
        case ResourceHarvestFailureKind::NodeTooFar:       return "node_too_far";
        case ResourceHarvestFailureKind::NodeDepleted:     return "node_depleted";
        case ResourceHarvestFailureKind::ActionMismatch:   return "action_mismatch";
        case ResourceHarvestFailureKind::ToolMissing:      return "tool_missing";
        case ResourceHarvestFailureKind::OutputInvalid:    return "output_invalid";
        case ResourceHarvestFailureKind::OutputSpawnFailed:return "output_spawn_failed";
        case ResourceHarvestFailureKind::RuntimeConflict:  return "runtime_conflict";
    }
    return "unknown";
}

std::string toString(HarvestOutputLocationPolicy policy) {
    switch (policy) {
        case HarvestOutputLocationPolicy::Unknown:          return "unknown";
        case HarvestOutputLocationPolicy::DropOnMap:        return "drop_on_map";
        case HarvestOutputLocationPolicy::PreferInventory:  return "prefer_inventory";
    }
    return "unknown";
}

// ---------------------------------------------------------------------------
// Validate basic
// ---------------------------------------------------------------------------

Result<void> ResourceHarvestRequest::validateBasic() const {
    if (harvest_id.empty()) {
        return Result<void>::fail(
            foundation::makeError(ErrorCode::command_invalid_argument,
                                  "invalid_request",
                                  "harvest_id is empty"));
    }
    if (actor_key.empty()) {
        return Result<void>::fail(
            foundation::makeError(ErrorCode::command_invalid_argument,
                                  "invalid_request",
                                  "actor_key is empty"));
    }
    if (node_id.empty()) {
        return Result<void>::fail(
            foundation::makeError(ErrorCode::command_invalid_argument,
                                  "invalid_request",
                                  "node_id is empty"));
    }
    if (harvest_kind == ResourceHarvestKind::Unknown) {
        return Result<void>::fail(
            foundation::makeError(ErrorCode::command_invalid_argument,
                                  "invalid_request",
                                  "harvest_kind is unknown"));
    }
    return Result<void>::ok();
}

} // namespace pathfinder::world_harvest
