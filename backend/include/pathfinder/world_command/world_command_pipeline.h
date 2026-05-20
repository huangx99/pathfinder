#pragma once

#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_projection.h"
#include "pathfinder/world_command/world_command_trace.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_command {

class WorldCommandPipeline {
public:
    explicit WorldCommandPipeline(WorldCommandDispatcher& dispatcher);

    pathfinder::foundation::Result<WorldCommandResponseDto> execute(
        const std::string& session_id,
        const WorldCommandDto& command);

    uint64_t currentProjectionVersion() const;
    void setCurrentProjectionVersion(uint64_t version);

private:
    WorldCommandDispatcher& dispatcher_;
    CommandTraceRecorder trace_recorder_;
    ProjectionPatchBuilder patch_builder_;
    AvailableCommandBuilder available_builder_;
    FrontendHintBuilder hint_builder_;
    uint64_t projection_version_ = 1;

    WorldCommandDto normalizeCommand(const WorldCommandDto& command);
    pathfinder::foundation::Result<void> validateCommandShape(const WorldCommandDto& command);
    pathfinder::foundation::Result<void> resolveActor(const WorldCommandDto& command);
    pathfinder::foundation::Result<void> resolveTarget(const WorldCommandDto& command);
    pathfinder::foundation::Result<void> validateSource(const WorldCommandDto& command);
    pathfinder::foundation::Result<void> validateLayer(const WorldCommandDto& command);
    pathfinder::foundation::Result<void> validateRange(const WorldCommandDto& command);
    pathfinder::foundation::Result<void> validateKnowledgeGate(const WorldCommandDto& command);
    pathfinder::foundation::Result<void> validateConditions(const WorldCommandDto& command);
    pathfinder::foundation::Result<void> validateMaterials(const WorldCommandDto& command);
};

} // namespace pathfinder::world_command
