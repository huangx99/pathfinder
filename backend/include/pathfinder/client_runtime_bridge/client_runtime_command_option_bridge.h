#pragma once

#include "pathfinder/client_runtime_bridge/client_runtime_bridge_port.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/world_harvest/resource_harvest_service.h"
#include "pathfinder/world_reaction/world_reaction_service.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/world_map_edit/map_edit_command_option_provider.h"
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
// 11.4 HarvestCommandOptionProvider
// ---------------------------------------------------------------------------

class HarvestCommandOptionProvider {
public:
    HarvestCommandOptionProvider(
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry,
        pathfinder::world_harvest::ResourceHarvestService& harvest_service);

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        const pathfinder::world_runtime::IWorldRuntime& runtime) const;

private:
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry_;
    pathfinder::world_harvest::ResourceHarvestService& harvest_service_;
};


// ---------------------------------------------------------------------------
// 11.5 CraftCommandOptionProvider
// ---------------------------------------------------------------------------

class CraftCommandOptionProvider {
public:
    CraftCommandOptionProvider(
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry,
        const pathfinder::content::ContentRegistry& content_registry,
        pathfinder::world_reaction::WorldReactionService& reaction_service);

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor) const;

private:
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry_;
    const pathfinder::content::ContentRegistry& content_registry_;
    pathfinder::world_reaction::WorldReactionService& reaction_service_;
};


// ---------------------------------------------------------------------------
// 11.6 TeachingAndNpcWorkCommandOptionProvider
// ---------------------------------------------------------------------------

class TeachingAndNpcWorkCommandOptionProvider {
public:
    TeachingAndNpcWorkCommandOptionProvider(
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry,
        const pathfinder::content::ContentRegistry& content_registry,
        pathfinder::knowledge::KnowledgeRepository& knowledge_repository);

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        const pathfinder::world_runtime::IWorldRuntime& runtime) const;

private:
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry_;
    const pathfinder::content::ContentRegistry& content_registry_;
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository_;
};

// ---------------------------------------------------------------------------
// 11.5 InventoryCommandOptionProvider
// ---------------------------------------------------------------------------

class InventoryCommandOptionProvider {
public:
    InventoryCommandOptionProvider(
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry,
        pathfinder::world_inventory::IInventoryRuntime& inventory_runtime);

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        const pathfinder::world_runtime::IWorldRuntime& runtime) const;

private:
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry_;
    pathfinder::world_inventory::IInventoryRuntime& inventory_runtime_;

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildPickupOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        const pathfinder::world_runtime::IWorldRuntime& runtime) const;

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildDropOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        const pathfinder::world_runtime::IWorldRuntime& runtime) const;
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

    // P60: Extended constructor with harvest and inventory services.
    ClientRuntimeCommandOptionBridge(
        pathfinder::world_runtime::IWorldRuntime& runtime,
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry,
        pathfinder::world_harvest::ResourceHarvestService* harvest_service,
        pathfinder::world_inventory::IInventoryRuntime* inventory_runtime,
        ClientRuntimeBridgeMode mode = ClientRuntimeBridgeMode::RuntimeBacked);

    void setCraftServices(
        pathfinder::world_reaction::WorldReactionService* reaction_service,
        std::shared_ptr<const pathfinder::content::ContentRegistry> content_registry);

    void setKnowledgeServices(
        pathfinder::knowledge::KnowledgeRepository* knowledge_repository,
        std::shared_ptr<const pathfinder::content::ContentRegistry> content_registry);

    void setMapEditServices(
        std::shared_ptr<const pathfinder::content::ContentRegistry> content_registry);

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
    std::unique_ptr<HarvestCommandOptionProvider> harvest_provider_;
    std::unique_ptr<CraftCommandOptionProvider> craft_provider_;
    std::unique_ptr<InventoryCommandOptionProvider> inventory_provider_;
    std::unique_ptr<TeachingAndNpcWorkCommandOptionProvider> teaching_provider_;
    std::shared_ptr<const pathfinder::content::ContentRegistry> craft_content_registry_;
    std::shared_ptr<const pathfinder::content::ContentRegistry> knowledge_content_registry_;
    pathfinder::knowledge::KnowledgeRepository* knowledge_repository_{nullptr};
    std::unique_ptr<pathfinder::world_map_edit::MapEditCommandOptionProvider> map_edit_provider_;
};

} // namespace pathfinder::client_runtime_bridge
