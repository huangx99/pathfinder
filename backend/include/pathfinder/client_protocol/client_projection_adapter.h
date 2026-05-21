#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::client_protocol {

// Summarizes backend safe projection into ClientWorldProjection.
// Does not leak hidden truth.
class ClientProjectionAdapter {
public:
    ClientProjectionAdapter();

    // Build a full safe projection for the given actor at the given version.
    // In P53 this is a stub; future phases will aggregate from runtime bridges.
    pathfinder::foundation::Result<ClientWorldProjection> buildFullProjection(
        const std::string& actor_key,
        const std::string& layer_key,
        uint64_t projection_version) const;

    // Build a scoped projection based on requested scopes.
    pathfinder::foundation::Result<ClientWorldProjection> buildScopedProjection(
        const std::string& actor_key,
        const std::string& layer_key,
        uint64_t projection_version,
        const std::vector<ClientProjectionScope>& scopes) const;

    // Merge a patch into an existing projection (for client-side simulation).
    // Returns a new projection; does not mutate the input.
    pathfinder::foundation::Result<ClientWorldProjection> mergePatch(
        const ClientWorldProjection& base,
        const WorldProjectionPatchDto& patch) const;
};

} // namespace pathfinder::client_protocol
