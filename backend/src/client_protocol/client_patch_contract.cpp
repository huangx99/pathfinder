#include "pathfinder/client_protocol/client_patch_contract.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::client_protocol {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

ClientPatchContract::ClientPatchContract() = default;

bool ClientPatchContract::canApplyPatch(
    uint64_t known_projection_version,
    uint64_t base_projection_version) const {
    // Exact match is required for local patch application.
    // If client is behind or ahead, they must full refresh.
    return known_projection_version == base_projection_version;
}

bool ClientPatchContract::requiresFullRefresh(
    const WorldProjectionPatchDto& patch,
    uint64_t known_projection_version) const {
    if (patch.requires_full_refresh) {
        return true;
    }
    if (!canApplyPatch(known_projection_version, patch.base_projection_version)) {
        return true;
    }
    return false;
}

Result<void> ClientPatchContract::validateVersionProgression(
    uint64_t base_version,
    uint64_t new_version) const {
    if (new_version < base_version) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed,
                "Projection version cannot decrease: base=" + std::to_string(base_version) +
                " new=" + std::to_string(new_version))
        );
    }
    return Result<void>::ok();
}

WorldProjectionPatchDto ClientPatchContract::buildFullRefreshHint(
    uint64_t known_projection_version,
    uint64_t new_projection_version) const {
    WorldProjectionPatchDto hint;
    hint.base_projection_version = known_projection_version;
    hint.new_projection_version = new_projection_version;
    hint.requires_full_refresh = true;
    return hint;
}

} // namespace pathfinder::client_protocol
