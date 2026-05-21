#pragma once

#include "pathfinder/client_runtime_bridge/client_runtime_bridge_port.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include <memory>

namespace pathfinder::client_runtime_bridge {

// ---------------------------------------------------------------------------
// 11.4 MovementCommandOptionProvider
// ---------------------------------------------------------------------------

class MovementCommandOptionProvider {
public:
    MovementCommandOptionProvider(
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry);

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        const pathfinder::world_runtime::IWorldRuntime& runtime) const;

private:
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry_;

    pathfinder::world_command::WorldCommandOptionDto buildMoveOption(
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        ClientMoveDirection direction,
        const pathfinder::world_runtime::IWorldRuntime& runtime) const;
};

// ---------------------------------------------------------------------------
// 11.5 InspectCommandOptionProvider
// ---------------------------------------------------------------------------

class InspectCommandOptionProvider {
public:
    InspectCommandOptionProvider(
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry);

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        const pathfinder::world_runtime::IWorldRuntime& runtime) const;

private:
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry_;
};

// ---------------------------------------------------------------------------
// 11.6 WaitCommandOptionProvider
// ---------------------------------------------------------------------------

class WaitCommandOptionProvider {
public:
    WaitCommandOptionProvider(
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry);

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor) const;

private:
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry_;
};

// ---------------------------------------------------------------------------
// 11.3 ClientRuntimeCommandOptionBridge
// ---------------------------------------------------------------------------
// Chinese: Generate available commands from real runtime context.
// Does not modify runtime. Only generates options for registered handlers.
// ---------------------------------------------------------------------------

class ClientRuntimeCommandOptionBridge : public IClientRuntimeBridgePort {
public:
    ClientRuntimeCommandOptionBridge(
        pathfinder::world_runtime::IWorldRuntime& runtime,
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry,
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
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry_;
    ClientRuntimeBridgeMode mode_;

    MovementCommandOptionProvider movement_provider_;
    InspectCommandOptionProvider inspect_provider_;
    WaitCommandOptionProvider wait_provider_;
};

} // namespace pathfinder::client_runtime_bridge
