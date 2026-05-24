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
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/world_harvest/resource_harvest_service.h"
#include "pathfinder/world_reaction/world_reaction_service.h"
#include "pathfinder/world_learning/world_knowledge_learning_service.h"
#include "pathfinder/learning/learning_loop.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/world_region_state/in_memory_world_region_state_store.h"
#include "pathfinder/world_region_state/world_grid_runtime_apply_port.h"
#include "pathfinder/world_region_state/world_region_lifecycle_service.h"
#include "pathfinder/world_region_state/world_region_ensure_service_adapter.h"
#include "pathfinder/world_map_interaction/region_activity_window_service.h"
#include "pathfinder/world_map_interaction/region_lifecycle_trigger_service.h"
#include "pathfinder/world_map_interaction/client_map_selection_service.h"
#include "pathfinder/world_map_interaction/client_map_projection_adapter.h"
#include "pathfinder/world_modules/core/world_module_context.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include <memory>
#include <string>

namespace pathfinder::client_runtime_host {

// P56: Creates the runtime-backed backend dependencies for client HTTP server.
// Wires P53 Client Protocol with WorldCommand pipeline and WorldGridRuntime.
//
// CRITICAL: Member declaration order determines initialization order.
// Any member used in another member's constructor must be declared BEFORE it.
struct ClientRuntimeHostFactory {
    // ------------------------------------------------------------------------
    // 1. Core runtime
    // ------------------------------------------------------------------------
    std::shared_ptr<pathfinder::world_runtime::WorldGridRuntime> world_runtime;
    std::shared_ptr<const pathfinder::content::ContentRegistry> content_registry;

    // ------------------------------------------------------------------------
    // 2. P60: Inventory and harvest infrastructure
    //    Must precede option_bridge (injected as raw pointers).
    // ------------------------------------------------------------------------
    std::shared_ptr<pathfinder::world_inventory::WorldInventoryRuntime> inventory_runtime;
    std::shared_ptr<pathfinder::world_harvest::ResourceHarvestService> harvest_service;
    std::shared_ptr<pathfinder::world_reaction::WorldReactionService> reaction_service;
    std::shared_ptr<pathfinder::learning::LearningLoopService> learning_service;
    std::shared_ptr<pathfinder::knowledge::KnowledgeRepository> knowledge_repository;
    std::shared_ptr<pathfinder::world_learning::WorldKnowledgeLearningService> knowledge_learning_service;

    // ------------------------------------------------------------------------
    // 3. P58: Region ensure infrastructure
    // ------------------------------------------------------------------------
    std::shared_ptr<pathfinder::world_generation::WorldGenerationService> worldgen_service;
    std::shared_ptr<pathfinder::world_generation::WorldRegionEnsureService> ensure_service;
    std::shared_ptr<pathfinder::client_runtime_bridge::ClientWorldRegionEnsureAdapter> ensure_adapter;
    std::shared_ptr<pathfinder::world_generation::MoveTargetRegionGuard> move_guard;

    // ------------------------------------------------------------------------
    // 4. Command registry (injected into option_bridge and dispatcher)
    // ------------------------------------------------------------------------
    pathfinder::world_command::WorldCommandHandlerRegistry registry;
    pathfinder::world_modules::WorldPostCommandHookRegistry post_command_hooks;

    // ------------------------------------------------------------------------
    // 5. Runtime bridges (depend on runtime and registry)
    // ------------------------------------------------------------------------
    std::shared_ptr<pathfinder::client_runtime_bridge::ClientRuntimeProjectionBridge> projection_bridge;
    std::shared_ptr<pathfinder::client_runtime_bridge::ClientRuntimeCommandOptionBridge> option_bridge;

    // ------------------------------------------------------------------------
    // 6. Protocol adapters (depend on bridges)
    // ------------------------------------------------------------------------
    pathfinder::client_protocol::ClientProjectionAdapter projection_adapter;
    pathfinder::client_protocol::ClientPatchContract patch_contract;
    pathfinder::client_protocol::ClientAvailableCommandAdapter available_command_adapter;

    // ------------------------------------------------------------------------
    // 7. P59/P60: Region lifecycle infrastructure
    // ------------------------------------------------------------------------
    std::shared_ptr<pathfinder::world_region_state::InMemoryWorldRegionStateStore> region_store;
    std::shared_ptr<pathfinder::world_region_state::WorldGridRuntimeApplyPort> apply_port;
    std::shared_ptr<pathfinder::world_region_state::WorldRegionEnsureServiceAdapter> ensure_service_adapter;
    std::shared_ptr<pathfinder::world_region_state::WorldRegionLifecycleService> lifecycle_service;

    // ------------------------------------------------------------------------
    // 8. P60: Map interaction services
    // ------------------------------------------------------------------------
    std::shared_ptr<pathfinder::world_map_interaction::RegionActivityWindowService> activity_window_service;
    std::shared_ptr<pathfinder::world_map_interaction::RegionLifecycleTriggerService> lifecycle_trigger_service;
    std::shared_ptr<pathfinder::world_map_interaction::ClientMapSelectionService> map_selection_service;
    std::shared_ptr<pathfinder::world_map_interaction::ClientMapProjectionAdapter> map_projection_adapter;

    // ------------------------------------------------------------------------
    // 9. Protocol layer (depends on adapters above)
    // ------------------------------------------------------------------------
    pathfinder::client_protocol::ClientSessionGateway session_gateway;
    pathfinder::world_command::WorldCommandDispatcher dispatcher;
    pathfinder::world_command::WorldCommandPipeline pipeline;
    pathfinder::client_protocol::ClientCommandGateway command_gateway;
    pathfinder::client_protocol::ClientProtocolCodec codec;
    std::unique_ptr<pathfinder::client_http::ClientHttpGateway> http_gateway;

    ClientRuntimeHostFactory();

    pathfinder::foundation::Result<void> resetWorld();
};

} // namespace pathfinder::client_runtime_host
