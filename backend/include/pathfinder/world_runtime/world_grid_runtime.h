#pragma once

#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/world_region_state/iworld_region_state_query_port.h"
#include <set>

namespace pathfinder::world_runtime {

class WorldGridRuntime final : public IWorldRuntime,
                                    public world_inventory::IWorldEntityLocationPort,
                                    public world_region_state::IWorldRegionStateQueryPort {
public:
    WorldGridRuntime();

    foundation::Result<void> initialize(const WorldRuntimeConfig& config) override;
    uint64_t currentWorldTick() const override;
    uint64_t stateVersion() const override;
    std::string worldId() const override;
    uint64_t worldSeed() const override;
    int regionSize() const override;
    std::string generatorVersion() const override;
    std::string contentVersion() const override;
    std::string worldgenProfileKey() const override;

    foundation::Result<const WorldCellRuntime*> findCell(const WorldCellCoord& coord) const override;
    foundation::Result<const WorldEntityInstance*> findEntity(const std::string& entity_id) const override;
    foundation::Result<const WorldActorRuntime*> findActor(const std::string& actor_key) const override;

    foundation::Result<void> generateInitialWorld(const WorldRuntimeConfig& config) override;

    // P57: Setup player actor and exploration without generating terrain.
    // Used by client_server when WorldGenerationService handles terrain.
    foundation::Result<void> setupPlayerActor(const WorldRuntimeConfig& config);
    foundation::Result<void> spawnActor(
        const std::string& actor_key,
        const std::string& entity_key,
        const std::string& display_name_key,
        const WorldCellCoord& coord,
        int vision_radius,
        bool is_player_controlled = false,
        const std::vector<std::string>& tag_keys = {},
        const std::map<std::string, double>& numeric_states = {},
        const std::string& entity_id_override = "") override;
    foundation::Result<MoveActorResult> moveActor(const std::string& actor_key, const WorldCellCoord& target) override;
    foundation::Result<ActorHealthChangeResult> applyActorHealthDelta(
        const std::string& actor_key,
        int delta,
        const std::vector<std::string>& reason_keys) override;
    foundation::Result<InspectWorldResult> inspect(const std::string& actor_key, const world_command::WorldCommandTargetDto& target) const override;
    foundation::Result<AdvanceWorldTimeResult> advanceWorldTime(uint64_t tick_delta) override;

    foundation::Result<WorldRuntimeSnapshot> snapshotForDebug() const override;
    foundation::Result<void> refreshActorExploration(const std::string& actor_key) override;

    // P46: Generated cell creation/updating
    foundation::Result<void> createOrUpdateGeneratedCell(
        const WorldCellCoord& coord,
        const std::string& terrain_key,
        const std::string& region_id,
        bool blocks_movement,
        int movement_cost,
        const std::vector<std::string>& tag_keys) override;

    // P46: Resource node runtime
    foundation::Result<void> upsertGeneratedResourceNode(const WorldResourceNodeRuntime& node) override;
    foundation::Result<const WorldResourceNodeRuntime*> findResourceNode(const std::string& node_id) const override;
    foundation::Result<std::vector<WorldResourceNodeRuntime>> queryResourceNodesAtCell(const WorldCellCoord& coord) const override;

    // P47: Update resource node after harvest
    foundation::Result<void> updateResourceNodeRuntime(const WorldResourceNodeRuntime& node) override;

    // P46: Region generation tracking
    bool isRegionGenerated(const std::string& region_id) const override;
    void markRegionGenerated(const std::string& region_id) override;

    // Exploration access for projection adapter
    WorldCellVisibility getCellVisibility(const std::string& actor_key, const std::string& cell_id) const;
    std::vector<std::string> getVisibleCellIds(const std::string& actor_key) const;

    // IWorldEntityLocationPort implementation (P45)
    foundation::Result<void> removeEntityFromMap(const std::string& entity_id) override;
    foundation::Result<void> placeEntityOnMap(const std::string& entity_id, const WorldCellCoord& coord) override;
    foundation::Result<void> setEntityLocationKind(const std::string& entity_id, WorldEntityLocationKind kind) override;
    foundation::Result<void> setEntityStackData(const std::string& entity_id, int quantity, const std::string& stack_key, bool stackable) override;
    foundation::Result<void> destroyEntity(const std::string& entity_id) override;
    bool isCellVisibleToActor(const std::string& actor_key, const WorldCellCoord& coord) const override;
    WorldCellVisibility getCellVisibilityForActor(const std::string& actor_key, const std::string& cell_id) const override;
    foundation::Result<void> spawnEntityOnMap(
        const std::string& entity_id,
        const std::string& entity_key,
        const std::string& display_name_key,
        const WorldCellCoord& coord,
        int quantity,
        const std::string& stack_key,
        bool stackable,
        const std::vector<std::string>& state_keys,
        const std::map<std::string, double>& numeric_states,
        const std::vector<std::string>& tag_keys) override;

    foundation::Result<void> spawnEntityInInventory(
        const std::string& entity_id,
        const std::string& entity_key,
        const std::string& display_name_key,
        const std::string& owner_ref,
        int quantity,
        const std::string& stack_key,
        bool stackable,
        const std::vector<std::string>& state_keys,
        const std::map<std::string, double>& numeric_states,
        const std::vector<std::string>& tag_keys) override;

    // P59: IWorldRegionStateQueryPort implementation
    foundation::Result<world_region_state::WorldRegionRuntimeSlice> readRegionSlice(
        const world_generation::WorldRegionKey& key) const override;

    // P59: Detach region from runtime (remove cells/entities/nodes for this region)
    foundation::Result<std::vector<std::string>> detachRegion(const std::string& region_id) override;
    foundation::Result<void> applyExplorationVisibility(
        const std::string& actor_key,
        const std::map<std::string, WorldCellVisibility>& cell_visibility_by_id) override;

private:
    WorldRuntimeConfig config_;
    uint64_t world_tick_ = 0;
    uint64_t state_version_ = 0;
    std::map<std::string, WorldCellRuntime> cells_;
    std::map<std::string, WorldEntityInstance> entities_;
    std::map<std::string, WorldActorRuntime> actors_;
    std::map<std::string, WorldExplorationState> exploration_;
    std::map<std::string, WorldResourceNodeRuntime> resource_nodes_;
    std::set<std::string> generated_regions_;

    void incrementStateVersion();
    void updateExplorationForActor(const std::string& actor_key);
    bool isAdjacent(const WorldCellCoord& a, const WorldCellCoord& b) const;
};

} // namespace pathfinder::world_runtime
