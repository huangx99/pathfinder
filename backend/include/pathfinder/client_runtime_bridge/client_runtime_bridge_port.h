#pragma once

#include "pathfinder/client_runtime_bridge/client_runtime_bridge_types.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::client_runtime_bridge {

// ---------------------------------------------------------------------------
// 11.1 IClientRuntimeBridgePort
// ---------------------------------------------------------------------------
// Chinese: client_protocol to real runtime read-only bridge interface.
// Read-only runtime. Does not modify world. Does not execute command.
// Repeatable. No random dependency. Returns structured error on failure.
// ---------------------------------------------------------------------------

class IClientRuntimeBridgePort {
public:
    virtual ~IClientRuntimeBridgePort() = default;

    virtual pathfinder::foundation::Result<ClientRuntimeView> buildRuntimeView(
        const ClientRuntimeViewRequest& request) const = 0;

    virtual pathfinder::foundation::Result<std::vector<pathfinder::world_command::WorldCommandOptionDto>> buildRuntimeOptions(
        const ClientRuntimeCommandOptionRequest& request) const = 0;

    virtual pathfinder::foundation::Result<ClientRuntimeBridgeDiagnostics> diagnostics(
        const std::string& actor_key,
        const std::string& layer_key) const = 0;
};

} // namespace pathfinder::client_runtime_bridge
