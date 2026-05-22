#pragma once

#include "pathfinder/client_protocol/client_projection_adapter.h"
#include "pathfinder/client_protocol/client_patch_contract.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/client_protocol/client_command_gateway.h"
#include "pathfinder/client_protocol/client_protocol_codec.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/client_runtime_bridge/client_runtime_projection_bridge.h"
#include "pathfinder/client_runtime_bridge/client_runtime_command_option_bridge.h"
#include "pathfinder/client_runtime_bridge/client_world_region_ensure_adapter.h"
#include "pathfinder/world_generation/move_target_region_guard.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include <memory>
#include <string>

namespace pathfinder::client_http {

// P56: Creates the runtime-backed backend dependencies for client HTTP server.
// Wires P53 Client Protocol with WorldCommand pipeline and WorldGridRuntime.
struct ClientServerRuntimeFactory {
    // Runtime
    std::shared_ptr<pathfinder::world_runtime::WorldGridRuntime> world_runtime;

    // Bridges (must outlive adapters)
    std::shared_ptr<pathfinder::client_runtime_bridge::ClientRuntimeProjectionBridge> projection_bridge;
    std::shared_ptr<pathfinder::client_runtime_bridge::ClientRuntimeCommandOptionBridge> option_bridge;

    // Protocol adapters (injected with bridges)
    pathfinder::client_protocol::ClientProjectionAdapter projection_adapter;
    pathfinder::client_protocol::ClientPatchContract patch_contract;
    pathfinder::client_protocol::ClientAvailableCommandAdapter available_command_adapter;

    // P58: Region ensure infrastructure (must outlive session_gateway and handlers)
    std::shared_ptr<pathfinder::world_generation::WorldGenerationService> worldgen_service;
    std::shared_ptr<pathfinder::world_generation::WorldRegionEnsureService> ensure_service;
    std::shared_ptr<pathfinder::client_runtime_bridge::ClientWorldRegionEnsureAdapter> ensure_adapter;
    std::shared_ptr<pathfinder::world_generation::MoveTargetRegionGuard> move_guard;

    pathfinder::client_protocol::ClientSessionGateway session_gateway;
    pathfinder::world_command::WorldCommandHandlerRegistry registry;
    pathfinder::world_command::WorldCommandDispatcher dispatcher;
    pathfinder::world_command::WorldCommandPipeline pipeline;
    pathfinder::client_protocol::ClientCommandGateway command_gateway;
    pathfinder::client_protocol::ClientProtocolCodec codec;
    std::unique_ptr<pathfinder::client_http::ClientHttpGateway> http_gateway;

    ClientServerRuntimeFactory();
};

} // namespace pathfinder::client_http
