#pragma once

#include "pathfinder/world_teaching/world_teaching_types.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/content/content_registry.h"
#include <vector>
#include <string>

namespace pathfinder::world_teaching {

// ---------------------------------------------------------------------------
// WorldTeachingProjectionBridge
// ---------------------------------------------------------------------------
// Converts teaching results into frontend-safe projection patches,
// events, and state deltas.
// ---------------------------------------------------------------------------

class WorldTeachingProjectionBridge {
public:
    explicit WorldTeachingProjectionBridge(
        const pathfinder::content::ContentRegistry* content_registry = nullptr);
    struct ProjectionResult {
        world_command::WorldProjectionPatchDto patch;
        std::vector<world_command::WorldEventDto> events;
        std::vector<world_command::WorldStateDeltaDto> state_deltas;
        std::vector<std::string> warning_keys;
    };

    ProjectionResult project(
        const std::vector<pathfinder::knowledge::KnowledgeClaim>& recipient_claims,
        const pathfinder::knowledge::KnowledgeClaim* source_claim,
        const std::string& teacher_actor_key,
        const std::string& recipient_actor_key,
        const std::string& source_knowledge_id,
        WorldTeachingDecision decision,
        uint64_t tick) const;

private:
    const pathfinder::content::ContentRegistry* content_registry_{nullptr};

    world_command::KnowledgePatchDto claimToPatch(
        const pathfinder::knowledge::KnowledgeClaim& claim,
        const std::string& recipient_actor_key) const;

    world_command::WorldEventDto makeTeachingEvent(
        const std::string& teacher_actor_key,
        const std::string& recipient_actor_key,
        const std::string& source_knowledge_id,
        uint64_t tick) const;

    world_command::WorldStateDeltaDto makeTeachingDelta(
        const pathfinder::knowledge::KnowledgeClaim& claim,
        const std::string& recipient_actor_key,
        const std::string& source_knowledge_id,
        WorldTeachingDecision decision) const;
};

} // namespace pathfinder::world_teaching
