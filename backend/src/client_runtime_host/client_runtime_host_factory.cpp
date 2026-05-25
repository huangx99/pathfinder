#include "pathfinder/client_runtime_host/client_runtime_host_factory.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_region_ensure_service.h"
#include "pathfinder/world_generation/move_target_region_guard.h"
#include "pathfinder/client_runtime_bridge/client_world_region_ensure_adapter.h"
#include "pathfinder/world_map_interaction/region_activity_window_service.h"
#include "pathfinder/world_map_interaction/region_lifecycle_trigger_service.h"
#include "pathfinder/world_map_interaction/client_map_selection_service.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"
#include "pathfinder/world_runtime/world_command_runtime_bridge.h"
#include "pathfinder/world_inventory/inventory_projection_adapter.h"
#include "pathfinder/world_harvest/harvest_projection_bridge.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/content/json_content_loader.h"
#include "pathfinder/world_modules/core/default_world_modules.h"
#include <iostream>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace pathfinder::client_runtime_host {

using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::world_runtime;
using namespace pathfinder::client_runtime_bridge;

namespace {



pathfinder::foundation::Result<void> initializeSandboxWorld(
    WorldGridRuntime& runtime,
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
    pathfinder::world_region_state::InMemoryWorldRegionStateStore* region_store,
    pathfinder::world_map_interaction::RegionActivityWindowService* activity_window_service) {

    WorldRuntimeConfig config;
    config.world_id = "world_default";
    config.seed = 1;
    config.region_size = 16;
    config.default_vision_radius = 32;
    config.worldgen_profile_key = "sandbox_blank";
    config.create_player_entity = false;
    config.player_is_controlled = false;

    auto init_res = runtime.initialize(config);
    if (init_res.is_error()) {
        return init_res;
    }

    const std::string region_id = config.world_id + ":surface:region:0:0:" + std::to_string(config.region_size);
    const int min_coord = -(config.region_size / 2);
    const int max_coord = (config.region_size / 2) - 1;
    for (int y = min_coord; y <= max_coord; ++y) {
        for (int x = min_coord; x <= max_coord; ++x) {
            auto cell_res = runtime.createOrUpdateGeneratedCell(
                WorldCellCoord{x, y, "surface"},
                "plain",
                region_id,
                false,
                1,
                {"plain", "sandbox"});
            if (cell_res.is_error()) {
                return cell_res;
            }
        }
    }
    runtime.markRegionGenerated(region_id);

    auto player_res = runtime.setupPlayerActor(config);
    if (player_res.is_error()) {
        return player_res;
    }

    auto inv_init = inventory_runtime.initialize();
    if (inv_init.is_error()) {
        return inv_init;
    }

    pathfinder::world_modules::clearDefaultWorldModuleRuntimeState();
    if (region_store) region_store->clear();
    if (activity_window_service) activity_window_service->clearTracking();

    return pathfinder::foundation::Result<void>::ok();
}

std::mutex& resetMutex() {
    static std::mutex mutex;
    return mutex;
}

std::filesystem::path findContentRoot() {
    auto find_from = [](std::filesystem::path current) -> std::filesystem::path {
        for (int i = 0; i < 10; ++i) {
            auto content_root = current / "content";
            if (std::filesystem::exists(content_root / "core" / "manifest.json")) {
                return content_root;
            }
            if (!current.has_parent_path() || current == current.parent_path()) break;
            current = current.parent_path();
        }
        return {};
    };

    if (auto from_cwd = find_from(std::filesystem::current_path()); !from_cwd.empty()) {
        return from_cwd;
    }

#ifdef PATHFINDER_REPO_ROOT
    {
        auto content_root = std::filesystem::path(PATHFINDER_REPO_ROOT) / "content";
        if (std::filesystem::exists(content_root / "core" / "manifest.json")) {
            return content_root;
        }
    }
#endif

    if (auto from_source = find_from(std::filesystem::path(__FILE__).parent_path()); !from_source.empty()) {
        return from_source;
    }

    return std::filesystem::path("content");
}


} // namespace

ClientRuntimeHostFactory::ClientRuntimeHostFactory()
    : world_runtime(std::make_shared<WorldGridRuntime>())
    , inventory_runtime(std::make_shared<pathfinder::world_inventory::WorldInventoryRuntime>(*world_runtime))
    , harvest_service(std::make_shared<pathfinder::world_harvest::ResourceHarvestService>(*world_runtime, *world_runtime, *inventory_runtime))
    , projection_bridge(std::make_shared<ClientRuntimeProjectionBridge>(*world_runtime, inventory_runtime.get(), ClientRuntimeBridgeMode::RuntimeBacked))
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

    reaction_service = std::make_shared<pathfinder::world_reaction::WorldReactionService>(
        *content_registry, *world_runtime, *inventory_runtime, *world_runtime);
    option_bridge->setCraftServices(reaction_service.get(), content_registry);

    learning_service = std::make_shared<pathfinder::learning::LearningLoopService>();
    knowledge_repository = std::make_shared<pathfinder::knowledge::KnowledgeRepository>();
    knowledge_learning_service = std::make_shared<pathfinder::world_learning::WorldKnowledgeLearningService>(
        *content_registry, *learning_service, *knowledge_repository);
    option_bridge->setKnowledgeServices(knowledge_repository.get(), content_registry);
    option_bridge->setMapEditServices(content_registry);

    command_gateway.setPostCommandHook([this](const WorldCommandDto& command, ClientCommandResponse& response) {
        const auto run_world_modules = [&]() {
            post_command_hooks.runHooks(command, response);
        };

        if (!knowledge_learning_service || !knowledge_repository || response.experiences.empty()) {
            run_world_modules();
            return;
        }

        pathfinder::world_learning::WorldKnowledgeLearningRequest request;
        request.request_id = response.result.command_id + "_learning";
        request.source_command = command;
        request.source_kind = pathfinder::world_learning::WorldLearningSourceKind::DirectExperience;
        request.actor_key = response.result.actor_key.empty() ? command.actor_key : response.result.actor_key;
        request.tick = response.new_projection_version;
        request.command_result.result_kind = response.result.result_kind;
        request.command_result.failure_reason_keys = response.result.failure_reason_keys;
        request.command_result.warning_keys = response.result.warning_keys;
        request.command_result.state_deltas = response.result.state_deltas;
        request.command_result.spawned_commands = response.result.spawned_commands;
        request.command_result.events = response.event_feed;
        request.command_result.experiences = response.experiences;

        pathfinder::knowledge::KnowledgeOwner owner;
        owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
        owner.entity_id = pathfinder::foundation::EntityId(request.actor_key);
        auto claims_res = knowledge_repository->listByOwner(owner);
        if (claims_res.is_ok()) {
            request.existing_actor_claims = claims_res.value();
        }

        auto learning_res = knowledge_learning_service->learnFromCommandResult(request);
        if (learning_res.is_error()) {
            response.warning_keys.push_back("knowledge_learning_failed");
            return;
        }
        auto learning = learning_res.value();
        response.warning_keys.insert(response.warning_keys.end(), learning.warning_keys.begin(), learning.warning_keys.end());
        response.event_feed.insert(response.event_feed.end(), learning.events.begin(), learning.events.end());
        response.result.state_deltas.insert(response.result.state_deltas.end(), learning.state_deltas.begin(), learning.state_deltas.end());
        if (learning.projection_patch.has_value()) {
            auto& patch = learning.projection_patch.value();
            response.projection_patch.changed_knowledge.insert(
                response.projection_patch.changed_knowledge.end(),
                patch.changed_knowledge.begin(),
                patch.changed_knowledge.end());
        }
        run_world_modules();
    });

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

    // P64: gameplay/runtime command handlers are registered by world modules,
    // not by the runtime host factory. The factory only supplies authoritative services.
    post_command_hooks.clear();
    pathfinder::world_modules::WorldModuleContext module_context{
        registry,
        pipeline,
        worldgen_service,
        move_guard.get(),
        *world_runtime,
        *inventory_runtime,
        *harvest_service,
        *reaction_service,
        *content_registry,
        *knowledge_repository,
        post_command_hooks};
    pathfinder::world_modules::registerDefaultWorldModules(module_context);

    // P60: Create region activity window service
    {
        world_map_interaction::RegionActivityWindowService::Config aw_config;
        aw_config.vision_radius_cells = 32;
        aw_config.preload_margin_regions = 1;
        // Local clients may advance the world automatically at a high frequency.
        // Keep generated runtime regions stable for the playable prototype so
        // already-discovered map objects do not appear to change or vanish merely
        // because passive world ticks accumulated quickly.
        aw_config.seal_delay_ticks = 600;
        aw_config.detach_delay_ticks = 600;
        aw_config.recently_touched_window_ticks = 600;
        activity_window_service = std::make_shared<world_map_interaction::RegionActivityWindowService>(
            aw_config, *world_runtime);
    }

    // P60: Create region lifecycle trigger service
    lifecycle_trigger_service = std::make_shared<world_map_interaction::RegionLifecycleTriggerService>(
        *lifecycle_service);
    lifecycle_trigger_service->setActivityWindowService(activity_window_service.get());

    auto world_reset_res = resetWorld();
    if (world_reset_res.is_error()) {
        std::cerr << "[P60] Initial sandbox world reset failed\n";
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
    http_gateway = std::make_unique<pathfinder::client_http::ClientHttpGateway>(
        codec,
        session_gateway,
        command_gateway,
        map_selection_service.get(),
        [this]() { return this->resetWorld(); });

}

pathfinder::foundation::Result<void> ClientRuntimeHostFactory::resetWorld() {
    std::lock_guard<std::mutex> lock(resetMutex());
    if (knowledge_repository) {
        knowledge_repository->clear();
    }
    return initializeSandboxWorld(
        *world_runtime,
        *inventory_runtime,
        region_store.get(),
        activity_window_service.get());
}

} // namespace pathfinder::client_runtime_host
