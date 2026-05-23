#pragma once

#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>

namespace pathfinder::world_learning {

// ---------------------------------------------------------------------------
// WorldKnowledgeProjectionBridge
// ---------------------------------------------------------------------------
// Converts knowledge learning results into frontend-safe projection patches,
// events, and state deltas. Human-readable summaries are derived from
// ContentRegistry so the frontend never infers recipe or knowledge rules.
// ---------------------------------------------------------------------------

class WorldKnowledgeProjectionBridge {
public:
    explicit WorldKnowledgeProjectionBridge(
        const pathfinder::content::ContentRegistry* content_registry = nullptr);

    struct ProjectionResult {
        world_command::WorldProjectionPatchDto patch;
        std::vector<world_command::WorldEventDto> events;
        std::vector<world_command::WorldStateDeltaDto> state_deltas;
        std::vector<std::string> warning_keys;
    };

    ProjectionResult project(
        const std::vector<pathfinder::knowledge::KnowledgeClaim>& learned_claims,
        const std::vector<WorldKnowledgeDelta>& knowledge_deltas,
        const std::string& actor_key,
        uint64_t tick) const;

private:
    const pathfinder::content::ContentRegistry* content_registry_{nullptr};

    world_command::KnowledgePatchDto claimToPatch(
        const pathfinder::knowledge::KnowledgeClaim& claim,
        const std::string& actor_key) const;

    world_command::WorldEventDto claimToEvent(
        const pathfinder::knowledge::KnowledgeClaim& claim,
        const std::string& actor_key,
        uint64_t tick) const;

    world_command::WorldStateDeltaDto claimToDelta(
        const WorldKnowledgeDelta& delta) const;
};

} // namespace pathfinder::world_learning
