#include "pathfinder/agent/binding.h"
#include "pathfinder/foundation/id.h"
#include <algorithm>
#include <set>

namespace pathfinder::agent {

foundation::Result<void> AgentBinding::validateBasic() const {
    if (agent_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "agent_binding_agent_id_missing", "agent_id must not be empty"));
    }
    if (!foundation::isValidIdString(agent_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "agent_binding_agent_id_invalid", "agent_id has invalid format: " + agent_id.value()));
    }
    if (primary_actor_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "agent_binding_primary_actor_id_missing", "primary_actor_id must not be empty"));
    }
    if (!foundation::isValidIdString(primary_actor_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "agent_binding_primary_actor_id_invalid", "primary_actor_id has invalid format: " + primary_actor_id.value()));
    }
    if (authority == AgentControlAuthority::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_binding_authority_unknown", "authority must not be Unknown"));
    }

    // Check for duplicates in controlled_actor_ids and validate ID format
    std::set<std::string> seen;
    for (const auto& id : controlled_actor_ids) {
        if (!foundation::isValidIdString(id.value())) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::id_invalid_format, "agent_binding_controlled_actor_id_invalid", "controlled_actor_ids contains invalid format: " + id.value()));
        }
        if (!seen.insert(id.value()).second) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::command_duplicate, "agent_binding_duplicate_actor", "controlled_actor_ids contains duplicate: " + id.value()));
        }
    }

    // If controlled_actor_ids is non-empty, it must contain primary_actor_id
    if (!controlled_actor_ids.empty()) {
        bool found = false;
        for (const auto& id : controlled_actor_ids) {
            if (id == primary_actor_id) {
                found = true;
                break;
            }
        }
        if (!found) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::command_invalid_argument, "agent_binding_primary_not_in_controlled",
                    "controlled_actor_ids must contain primary_actor_id"));
        }
    }

    return foundation::Result<void>::ok();
}

} // namespace pathfinder::agent
