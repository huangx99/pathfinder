#include "pathfinder/world_command/world_command_projection.h"

namespace pathfinder::world_command {

using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// ProjectionPatchBuilder
// ---------------------------------------------------------------------------
ProjectionPatchBuilder::ProjectionPatchBuilder() = default;

ProjectionPatchBuilder& ProjectionPatchBuilder::setVersions(uint64_t base_version, uint64_t new_version) {
    base_version_ = base_version;
    new_version_ = new_version;
    return *this;
}

ProjectionPatchBuilder& ProjectionPatchBuilder::setRequiresFullRefresh(bool value) {
    requires_full_refresh_ = value;
    return *this;
}

Result<WorldProjectionPatchDto> ProjectionPatchBuilder::build() const {
    WorldProjectionPatchDto patch;
    patch.base_projection_version = base_version_;
    patch.new_projection_version = new_version_;
    patch.requires_full_refresh = requires_full_refresh_;
    return Result<WorldProjectionPatchDto>::ok(std::move(patch));
}

// ---------------------------------------------------------------------------
// AvailableCommandBuilder
// ---------------------------------------------------------------------------
AvailableCommandBuilder::AvailableCommandBuilder() = default;

Result<std::vector<WorldCommandOptionDto>> AvailableCommandBuilder::build(
    const WorldCommandContext& /*context*/) const {
    // P43 stub: returns empty available commands
    return Result<std::vector<WorldCommandOptionDto>>::ok(std::vector<WorldCommandOptionDto>{});
}

// ---------------------------------------------------------------------------
// FrontendHintBuilder
// ---------------------------------------------------------------------------
FrontendHintBuilder::FrontendHintBuilder() = default;

Result<std::vector<WorldFrontendHintDto>> FrontendHintBuilder::build(
    const WorldCommandContext& /*context*/,
    const WorldCommandExecutionResult& /*execution_result*/) const {
    // P43 stub: returns empty hints
    return Result<std::vector<WorldFrontendHintDto>>::ok(std::vector<WorldFrontendHintDto>{});
}

} // namespace pathfinder::world_command
