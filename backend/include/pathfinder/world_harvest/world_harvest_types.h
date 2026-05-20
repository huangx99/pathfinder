#pragma once

#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/world_command/world_command_types.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace pathfinder::world_harvest {

// ---------------------------------------------------------------------------
// 7. Core Enums
// ---------------------------------------------------------------------------

enum class ResourceHarvestKind {
    Unknown,
    Gather,
    Chop,
    Mine,
    Dig
};

std::string toString(ResourceHarvestKind kind);
ResourceHarvestKind harvestKindFromCommandKind(world_command::WorldCommandKind kind);

enum class ResourceHarvestFailureKind {
    None,
    InvalidRequest,
    ActorMissing,
    NodeMissing,
    NodeNotVisible,
    NodeTooFar,
    NodeDepleted,
    ActionMismatch,
    ToolMissing,
    OutputInvalid,
    OutputSpawnFailed,
    RuntimeConflict
};

std::string toString(ResourceHarvestFailureKind kind);

enum class HarvestOutputLocationPolicy {
    Unknown,
    DropOnMap,
    PreferInventory
};

std::string toString(HarvestOutputLocationPolicy policy);

// ---------------------------------------------------------------------------
// 8. Core DTOs
// ---------------------------------------------------------------------------

struct ResourceHarvestRequest {
    std::string harvest_id;
    std::string actor_key;
    std::string node_id;
    ResourceHarvestKind harvest_kind = ResourceHarvestKind::Unknown;
    HarvestOutputLocationPolicy output_location_policy = HarvestOutputLocationPolicy::DropOnMap;
    std::map<std::string, std::string> parameters;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ResourceHarvestOutputDraft {
    std::string entity_id;
    std::string entity_key;
    std::string display_name_key;
    world_runtime::WorldCellCoord coord;
    int quantity = 1;
    bool stackable = true;
    std::string stack_key;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
    std::vector<std::string> tag_keys;
};

struct ResourceHarvestDraft {
    std::string draft_id;
    ResourceHarvestRequest request;
    world_runtime::WorldResourceNodeRuntime node_before;
    std::vector<ResourceHarvestOutputDraft> output_drafts;
    int charges_to_consume = 1;
    bool will_deplete = false;
    std::vector<std::string> changed_cell_ids;
};

struct ResourceHarvestResult {
    bool ok = false;
    ResourceHarvestFailureKind failure_kind = ResourceHarvestFailureKind::None;
    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> changed_resource_node_ids;
    std::vector<world_command::WorldStateDeltaDto> state_deltas;
    std::vector<world_command::WorldEventDto> events;
};

} // namespace pathfinder::world_harvest
