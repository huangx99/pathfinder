#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/client_runtime_bridge/client_runtime_bridge_port.h"
#include "pathfinder/foundation/result.h"
#include <memory>

namespace pathfinder::client_protocol {

// Summarizes backend safe projection into ClientWorldProjection.
// Does not leak hidden truth.
class ClientProjectionAdapter {
public:
    ClientProjectionAdapter();
    explicit ClientProjectionAdapter(
        std::shared_ptr<pathfinder::client_runtime_bridge::IClientRuntimeBridgePort> bridge_port);

    // Build a full safe projection for the given actor at the given version.
    // P56: uses runtime bridge if injected; otherwise falls back to stub for tests.
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

private:
    std::shared_ptr<pathfinder::client_runtime_bridge::IClientRuntimeBridgePort> bridge_port_;

    pathfinder::foundation::Result<ClientWorldProjection> buildFromBridge(
        const std::string& actor_key,
        const std::string& layer_key,
        uint64_t projection_version,
        pathfinder::client_runtime_bridge::ClientProjectionBuildReason reason =
            pathfinder::client_runtime_bridge::ClientProjectionBuildReason::Bootstrap) const;
};

} // namespace pathfinder::client_protocol
