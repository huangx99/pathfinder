#include "pathfinder/client_http/client_server_runtime_factory.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_runtime/world_command_runtime_handlers.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_generation/world_generation_command_handler.h"
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
    config.seed = 1;
    config.region_size = 9;
    config.default_vision_radius = 12;

    auto init_res = world_runtime->initialize(config);
    if (init_res.is_error()) {
        std::cerr << "[P56] World runtime initialization failed\n";
    }

    // P57: Generate terrain via WorldGenerationService.
    // Preload an 11x11 region grid so the player can explore a large area.
    auto worldgen_service = std::make_shared<world_generation::WorldGenerationService>();
    int total_cells = 0;
    for (int rx = -5; rx <= 5; ++rx) {
        for (int ry = -5; ry <= 5; ++ry) {
            world_generation::WorldGenerationRequest wg_request;
            wg_request.world_id = config.world_id;
            wg_request.world_seed = config.seed;
            wg_request.generator_version = "1.0.0";
            wg_request.content_version = "1.0.0";
            wg_request.worldgen_profile_key = "first_world";
            wg_request.region_coord = world_generation::WorldRegionCoord{rx, ry};
            wg_request.region_size = config.region_size;
            wg_request.enabled_layer_keys = std::vector<std::string>{"surface"};

            auto wg_result = worldgen_service->generate(wg_request);
            if (wg_result.ok && !wg_result.cell_drafts.empty()) {
                world_generation::WorldGenerationApplier applier(*world_runtime, *world_runtime);
                auto apply_result = applier.apply(wg_result);
                if (apply_result.ok) {
                    total_cells += static_cast<int>(wg_result.cell_drafts.size());
                } else {
                    std::cerr << "[P57] Applier failed for region " << rx << "," << ry << "\n";
                }
            } else {
                std::cerr << "[P57] Generation failed for region " << rx << "," << ry << "\n";
            }
        }
    }
    std::cerr << "[P57] Total cells generated: " << total_cells << "\n";

    // Setup player actor after terrain is applied so origin cell exists.
    auto player_res = world_runtime->setupPlayerActor(config);
    if (player_res.is_error()) {
        std::cerr << "[P57] Player actor setup failed\n";
    }

    // Register runtime-aware handlers (P56)
    registry.registerHandler(createGenerateWorldCommandHandler(worldgen_service, *world_runtime, *world_runtime));
    registry.registerHandler(createMoveCommandHandler(*world_runtime));
    registry.registerHandler(createInspectCommandHandler(*world_runtime));
    registry.registerHandler(createWaitCommandHandler(*world_runtime));

    // Register minimal stub handlers for commands without runtime-aware versions
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createSystemTickCommandHandler());

    http_gateway = std::make_unique<ClientHttpGateway>(codec, session_gateway, command_gateway);
}

} // namespace pathfinder::client_http
