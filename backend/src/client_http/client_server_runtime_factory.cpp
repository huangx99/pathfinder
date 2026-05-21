#include "pathfinder/client_http/client_server_runtime_factory.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_runtime/world_command_runtime_handlers.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/foundation/error.h"
#include <iostream>

namespace pathfinder::client_http {

using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::world_runtime;
using namespace pathfinder::client_runtime_bridge;

ClientServerRuntimeFactory::ClientServerRuntimeFactory()
    : world_runtime(std::make_shared<WorldGridRuntime>())
    , projection_bridge(std::make_shared<ClientRuntimeProjectionBridge>(*world_runtime, ClientRuntimeBridgeMode::RuntimeBacked))
    , option_bridge(std::make_shared<ClientRuntimeCommandOptionBridge>(*world_runtime, registry, ClientRuntimeBridgeMode::RuntimeBacked))
    , projection_adapter(projection_bridge)
    , available_command_adapter(option_bridge)
    , session_gateway(projection_adapter, available_command_adapter)
    , dispatcher(registry)
    , pipeline(dispatcher)
    , command_gateway(session_gateway, pipeline, patch_contract, available_command_adapter) {

    // Initialize world runtime
    WorldRuntimeConfig config;
    config.world_id = "world_default";
    config.seed = 42;
    config.region_size = 9;
    config.default_vision_radius = 3;

    auto init_res = world_runtime->initialize(config);
    if (init_res.is_error()) {
        std::cerr << "[P56] World runtime initialization failed\n";
    }

    auto gen_res = world_runtime->generateInitialWorld(config);
    if (gen_res.is_error()) {
        std::cerr << "[P56] World generation failed\n";
    }

    // Register runtime-aware handlers (P56)
    registry.registerHandler(createGenerateWorldCommandHandler(*world_runtime));
    registry.registerHandler(createMoveCommandHandler(*world_runtime));
    registry.registerHandler(createInspectCommandHandler(*world_runtime));
    registry.registerHandler(createWaitCommandHandler(*world_runtime));

    // Register minimal stub handlers for commands without runtime-aware versions
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createSystemTickCommandHandler());

    http_gateway = std::make_unique<ClientHttpGateway>(codec, session_gateway, command_gateway);
}

} // namespace pathfinder::client_http
