#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::client_protocol {

// Validates projection version consistency and defines patch semantics.
class ClientPatchContract {
public:
    ClientPatchContract();

    // Check if the client's known version matches the server's base version.
    // Returns false if the client must perform a full refresh.
    bool canApplyPatch(
        uint64_t known_projection_version,
        uint64_t base_projection_version) const;

    // Determine whether a full refresh is required after receiving a response.
    bool requiresFullRefresh(
        const WorldProjectionPatchDto& patch,
        uint64_t known_projection_version) const;

    // Validate that the new version strictly increases (or stays same for no-op).
    pathfinder::foundation::Result<void> validateVersionProgression(
        uint64_t base_version,
        uint64_t new_version) const;

    // Build a full-refresh hint patch.
    WorldProjectionPatchDto buildFullRefreshHint(
        uint64_t known_projection_version,
        uint64_t new_projection_version) const;
};

} // namespace pathfinder::client_protocol
