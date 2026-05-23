#include "pathfinder/client_http/client_server_runtime_factory.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_runtime/world_command_runtime_handlers.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_generation/world_generation_command_handler.h"
#include "pathfinder/world_generation/world_region_ensure_service.h"
#include "pathfinder/world_generation/move_target_region_guard.h"
#include "pathfinder/client_runtime_bridge/client_world_region_ensure_adapter.h"
#include "pathfinder/world_harvest/harvest_command_handler.h"
#include "pathfinder/world_inventory/world_inventory_command_handlers.h"
#include "pathfinder/world_map_interaction/region_activity_window_service.h"
#include "pathfinder/world_map_interaction/region_lifecycle_trigger_service.h"
#include "pathfinder/world_map_interaction/client_map_selection_service.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/content/json_content_loader.h"
#include <iostream>
#include <filesystem>
#include <mutex>
#include <stdexcept>

namespace pathfinder::client_http {

using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::world_runtime;
using namespace pathfinder::client_runtime_bridge;

namespace {

pathfinder::foundation::Result<void> initializePlayableWorld(
    WorldGridRuntime& runtime,
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
    pathfinder::world_generation::WorldGenerationService& worldgen_service,
    pathfinder::world_region_state::InMemoryWorldRegionStateStore* region_store,
    pathfinder::world_map_interaction::RegionActivityWindowService* activity_window_service) {

    WorldRuntimeConfig config;
    config.world_id = "world_default";
    config.seed = 1;
    config.region_size = 16;
    config.default_vision_radius = 10;

    auto init_res = runtime.initialize(config);
    if (init_res.is_error()) {
        return init_res;
    }

    world_generation::WorldGenerationRequest wg_request;
    wg_request.world_id = config.world_id;
    wg_request.world_seed = config.seed;
    wg_request.generator_version = "1.0.0";
    wg_request.content_version = "1.0.0";
    wg_request.worldgen_profile_key = "first_world";
    wg_request.region_coord = world_generation::WorldRegionCoord{0, 0};
    wg_request.region_size = config.region_size;
    wg_request.enabled_layer_keys = std::vector<std::string>{"surface"};

    auto wg_result = worldgen_service.generate(wg_request);
    if (!wg_result.ok || wg_result.cell_drafts.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::state_change_invalid,
                "origin_region_generation_failed", "Failed to generate origin region"));
    }

    world_generation::WorldGenerationApplier applier(runtime, runtime);
    auto apply_result = applier.apply(wg_result);
    if (!apply_result.ok) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::state_change_invalid,
                "origin_region_apply_failed", "Failed to apply origin region"));
    }

    auto player_res = runtime.setupPlayerActor(config);
    if (player_res.is_error()) {
        return player_res;
    }

    auto inv_init = inventory_runtime.initialize();
    if (inv_init.is_error()) {
        return inv_init;
    }

    if (region_store) region_store->clear();
    if (activity_window_service) activity_window_service->clearTracking();

    return pathfinder::foundation::Result<void>::ok();
}

std::mutex& resetMutex() {
    static std::mutex mutex;
    return mutex;
}

std::filesystem::path findContentRoot() {
    auto current = std::filesystem::current_path();
    for (int i = 0; i < 8; ++i) {
        auto content_root = current / "content";
        if (std::filesystem::exists(content_root / "core" / "manifest.json")) {
            return content_root;
        }
        if (!current.has_parent_path() || current == current.parent_path()) break;
        current = current.parent_path();
    }
    return std::filesystem::path("content");
}


} // namespace

ClientServerRuntimeFactory::ClientServerRuntimeFactory()
    : world_runtime(std::make_shared<WorldGridRuntime>())
    , inventory_runtime(std::make_shared<pathfinder::world_inventory::WorldInventoryRuntime>(*world_runtime))
    , harvest_service(std::make_shared<pathfinder::world_harvest::ResourceHarvestService>(*world_runtime, *world_runtime, *inventory_runtime))
    , projection_bridge(std::make_shared<ClientRuntimeProjectionBridge>(*world_runtime, ClientRuntimeBridgeMode::RuntimeBacked))
    , option_bridge(std::make_shared<ClientRuntimeCommandOptionBridge>(*world_runtime, registry, harvest_service.get(), inventory_runtime.get(), ClientRuntimeBridgeMode::RuntimeBacked))
    , projection_adapter(projection_bridge)
    , available_command_adapter(option_bridge)
    , session_gateway(projection_adapter, available_command_adapter)
    , dispatcher(registry)
    , pipeline(dispatcher)
    , command_gateway(session_gateway, pipeline, patch_contract, available_command_adapter) {

    // P58/P62: Create world generation service and load content-backed profiles.
    worldgen_service = std::make_shared<world_generation::WorldGenerationService>();

    {
        pathfinder::content::JsonContentLoader loader;
        pathfinder::content::ContentLoadOptions options;
        options.root_path = findContentRoot().string();
        options.enabled_package_keys = {"core"};
        options.load_mode = pathfinder::content::ContentLoadMode::StrictContentRequired;
        auto content_load = loader.load(options);
        if (content_load.is_ok() && content_load.value().registry) {
            content_registry = content_load.value().registry;
            harvest_service->setContentRegistry(content_registry);
            auto register_res = worldgen_service->registerContentProfiles(*content_registry);
            if (register_res.is_error()) {
                throw std::runtime_error("[P62] Failed to register content worldgen profiles");
            }
        } else {
            throw std::runtime_error("[P62] Failed to load required core content registry");
        }
    }

    ensure_service = std::make_shared<world_generation::WorldRegionEnsureService>(
        *worldgen_service, *world_runtime, *world_runtime);
    ensure_adapter = std::make_shared<client_runtime_bridge::ClientWorldRegionEnsureAdapter>(
        *ensure_service, *world_runtime);
    move_guard = std::make_shared<world_generation::MoveTargetRegionGuard>(*ensure_service);

    // P59/P60: Create region lifecycle infrastructure
    region_store = std::make_shared<pathfinder::world_region_state::InMemoryWorldRegionStateStore>();
    apply_port = std::make_shared<pathfinder::world_region_state::WorldGridRuntimeApplyPort>(*world_runtime, *world_runtime);
    ensure_service_adapter = std::make_shared<pathfinder::world_region_state::WorldRegionEnsureServiceAdapter>(*ensure_service);
    lifecycle_service = std::make_shared<pathfinder::world_region_state::WorldRegionLifecycleService>(
        *region_store, *world_runtime, *apply_port, *world_runtime, *world_runtime, ensure_service_adapter.get());

    // P60: Wire lifecycle service into ensure adapter so restores are reported correctly.
    ensure_adapter->setLifecycleService(lifecycle_service.get());

    // P58: Inject ensure adapter into session gateway and command gateway.
    session_gateway.setRegionEnsureAdapter(ensure_adapter.get());
    command_gateway.setRegionEnsureAdapter(ensure_adapter.get());

    // Register runtime-aware handlers (P56)
    registry.registerHandler(createGenerateWorldCommandHandler(worldgen_service, *world_runtime, *world_runtime));
    registry.registerHandler(createMoveCommandHandler(*world_runtime, move_guard.get()));
    registry.registerHandler(createInspectCommandHandler(*world_runtime));
    registry.registerHandler(createWaitCommandHandler(*world_runtime));

    // P60: Register harvest handlers
    registry.registerHandler(pathfinder::world_command::createGatherCommandHandler(*harvest_service));
    registry.registerHandler(pathfinder::world_command::createChopCommandHandler(*harvest_service));
    registry.registerHandler(pathfinder::world_command::createMineCommandHandler(*harvest_service));
    registry.registerHandler(pathfinder::world_command::createDigCommandHandler(*harvest_service));

    // P60: Register inventory handlers
    registry.registerHandler(pathfinder::world_inventory::createPickupCommandHandler(*inventory_runtime, *world_runtime));
    registry.registerHandler(pathfinder::world_inventory::createDropCommandHandler(*inventory_runtime, *world_runtime));

    // Register base system handlers. These are not gameplay fallbacks and should
    // not be used to fake missing content actions; real playable behavior must have
    // a runtime-aware handler/provider wired through the Command pipeline.
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createSystemTickCommandHandler());

    // P60: Create region activity window service
    {
        world_map_interaction::RegionActivityWindowService::Config aw_config;
        aw_config.vision_radius_cells = 10;
        aw_config.preload_margin_regions = 1;
        aw_config.seal_delay_ticks = 10;
        aw_config.detach_delay_ticks = 3;
        aw_config.recently_touched_window_ticks = 10;
        activity_window_service = std::make_shared<world_map_interaction::RegionActivityWindowService>(
            aw_config, *world_runtime);
    }

    // P60: Create region lifecycle trigger service
    lifecycle_trigger_service = std::make_shared<world_map_interaction::RegionLifecycleTriggerService>(
        *lifecycle_service);
    lifecycle_trigger_service->setActivityWindowService(activity_window_service.get());

    auto world_reset_res = resetWorld();
    if (world_reset_res.is_error()) {
        std::cerr << "[P60] Initial playable world reset failed\n";
    }

    // P60: Create map selection service
    map_selection_service = std::make_shared<world_map_interaction::ClientMapSelectionService>(
        *world_runtime, *world_runtime, available_command_adapter);

    // P60: Create map projection adapter
    map_projection_adapter = std::make_shared<world_map_interaction::ClientMapProjectionAdapter>(
        projection_adapter, *world_runtime);

    // P60: Inject activity window and lifecycle trigger services into gateways.
    session_gateway.setActivityWindowService(activity_window_service.get());
    session_gateway.setLifecycleTriggerService(lifecycle_trigger_service.get());
    session_gateway.setWorldContext("world_default", 1);
    command_gateway.setActivityWindowService(activity_window_service.get());
    command_gateway.setLifecycleTriggerService(lifecycle_trigger_service.get());
    command_gateway.setWorldContext("world_default", 1);

    // P60: Create HTTP gateway with selection service
    http_gateway = std::make_unique<ClientHttpGateway>(
        codec,
        session_gateway,
        command_gateway,
        map_selection_service.get(),
        [this]() { return this->resetWorld(); });

}

pathfinder::foundation::Result<void> ClientServerRuntimeFactory::resetWorld() {
    std::lock_guard<std::mutex> lock(resetMutex());
    return initializePlayableWorld(
        *world_runtime,
        *inventory_runtime,
        *worldgen_service,
        region_store.get(),
        activity_window_service.get());
}

} // namespace pathfinder::client_http
