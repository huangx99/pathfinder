#pragma once

#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include <set>
#include <set>

namespace pathfinder::world_runtime {

class WorldGridRuntime final : public IWorldRuntime, public world_inventory::IWorldEntityLocationPort {
public:
    WorldGridRuntime();

    foundation::Result<void> initialize(const WorldRuntimeConfig& config) override;
    uint64_t currentWorldTick() const override;
    uint64_t stateVersion() const override;

    foundation::Result<const WorldCellRuntime*> findCell(const WorldCellCoord& coord) const override;
    foundation::Result<const WorldEntityInstance*> findEntity(const std::string& entity_id) const override;
    foundation::Result<const WorldActorRuntime*> findActor(const std::string& actor_key) const override;

    foundation::Result<void> generateInitialWorld(const WorldRuntimeConfig& config) override;
    foundation::Result<MoveActorResult> moveActor(const std::string& actor_key, const WorldCellCoord& target) override;
    foundation::Result<InspectWorldResult> inspect(const std::string& actor_key, const world_command::WorldCommandTargetDto& target) const override;
    foundation::Result<AdvanceWorldTimeResult> advanceWorldTime(uint64_t tick_delta) override;

    foundation::Result<WorldRuntimeSnapshot> snapshotForDebug() const override;

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
