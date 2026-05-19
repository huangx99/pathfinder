#pragma once

#include "pathfinder/world_interaction/world_types.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include <vector>

namespace pathfinder::world_interaction {

class WorldSnapshotAdapter {
public:
    pathfinder::foundation::Result<WorldSnapshot> fromDialogSession(
        const pathfinder::h5_dialog::DialogScenario& scenario,
        const pathfinder::h5_dialog::DialogSessionState& state) const;

    pathfinder::foundation::Result<void> writeBackToDialogSession(
        const WorldSnapshot& snapshot,
        pathfinder::h5_dialog::DialogSessionState& state) const;
};

class WorldInteractionService {
public:
    pathfinder::foundation::Result<WorldInteractionResult> resolve(
        const WorldSnapshot& snapshot,
        const WorldInteractionInput& input,
        const InteractionResolveOptions& options) const;
};

class WorldChangeApplier {
public:
    pathfinder::foundation::Result<WorldSnapshot> apply(
        const WorldSnapshot& before,
        const std::vector<WorldChange>& changes) const;
};

class AgentAutonomyService {
public:
    pathfinder::foundation::Result<AgentAutonomyResult> tryAct(
        const WorldSnapshot& snapshot,
        const AgentAutonomyRequest& request) const;
};

class ThreatEventBridge {
public:
    pathfinder::foundation::Result<std::vector<WorldChange>> progressThreats(
        const WorldSnapshot& snapshot,
        const ThreatProgressInput& input) const;

    pathfinder::foundation::Result<std::vector<WorldChange>> applyCountermeasure(
        const WorldSnapshot& snapshot,
        const ThreatCountermeasureInput& input) const;
};

class WorldProjectionMapper {
public:
    pathfinder::foundation::Result<WorldProjectionPatch> buildPatch(
        const WorldSnapshot& snapshot,
        const std::vector<WorldChange>& recent_changes,
        const std::vector<AgentAutonomyResult>& agent_results) const;
};

} // namespace pathfinder::world_interaction
