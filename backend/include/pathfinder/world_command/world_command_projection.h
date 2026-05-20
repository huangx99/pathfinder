#pragma once

#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_command {

class ProjectionPatchBuilder {
public:
    ProjectionPatchBuilder();

    ProjectionPatchBuilder& setVersions(uint64_t base_version, uint64_t new_version);
    ProjectionPatchBuilder& setRequiresFullRefresh(bool value);

    pathfinder::foundation::Result<WorldProjectionPatchDto> build() const;

private:
    uint64_t base_version_ = 0;
    uint64_t new_version_ = 0;
    bool requires_full_refresh_ = false;
};

class AvailableCommandBuilder {
public:
    AvailableCommandBuilder();

    pathfinder::foundation::Result<std::vector<WorldCommandOptionDto>> build(
        const WorldCommandContext& context) const;
};

class FrontendHintBuilder {
public:
    FrontendHintBuilder();

    pathfinder::foundation::Result<std::vector<WorldFrontendHintDto>> build(
        const WorldCommandContext& context,
        const WorldCommandExecutionResult& execution_result) const;
};

} // namespace pathfinder::world_command
