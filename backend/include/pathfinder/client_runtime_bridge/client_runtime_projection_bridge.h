#pragma once

#include "pathfinder/client_runtime_bridge/client_runtime_bridge_port.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"

namespace pathfinder::client_runtime_bridge {

// ---------------------------------------------------------------------------
// 11.2 ClientRuntimeProjectionBridge
// ---------------------------------------------------------------------------
// Chinese: Build ClientRuntimeView from IWorldRuntime.
// Only exposes safe, visible data. Never leaks hidden truth.
// ---------------------------------------------------------------------------

class ClientRuntimeProjectionBridge : public IClientRuntimeBridgePort {
public:
    explicit ClientRuntimeProjectionBridge(
        pathfinder::world_runtime::IWorldRuntime& runtime,
        ClientRuntimeBridgeMode mode = ClientRuntimeBridgeMode::RuntimeBacked);

    pathfinder::foundation::Result<ClientRuntimeView> buildRuntimeView(
        const ClientRuntimeViewRequest& request) const override;

    pathfinder::foundation::Result<std::vector<pathfinder::world_command::WorldCommandOptionDto>> buildRuntimeOptions(
        const ClientRuntimeCommandOptionRequest& request) const override;

    pathfinder::foundation::Result<ClientRuntimeBridgeDiagnostics> diagnostics(
        const std::string& actor_key,
        const std::string& layer_key) const override;

private:
    pathfinder::world_runtime::IWorldRuntime& runtime_;
    pathfinder::world_runtime::WorldProjectionAdapter projection_adapter_;
    ClientRuntimeBridgeMode mode_;

    pathfinder::foundation::Result<std::vector<pathfinder::world_command::WorldCellPatchDto>> buildVisibleCells(
        const std::string& actor_key,
        const std::string& layer_key) const;

    pathfinder::foundation::Result<std::vector<pathfinder::world_command::WorldEntityPatchDto>> buildVisibleEntities(
        const std::string& actor_key,
        const std::string& layer_key) const;
};

} // namespace pathfinder::client_runtime_bridge
