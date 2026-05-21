#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/content/content_types.h"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::world_reaction {

// ---------------------------------------------------------------------------
// Core Enums
// ---------------------------------------------------------------------------

enum class WorldReactionActionKind {
    Unknown,
    Craft,
    Use,
    Combine
};

std::string toString(WorldReactionActionKind kind);
pathfinder::foundation::Result<WorldReactionActionKind> worldReactionActionKindFromString(const std::string& str);

enum class WorldReactionInputSourceKind {
    Unknown,
    ActorInventory,
    SameCellMap,
    AdjacentMap
};

std::string toString(WorldReactionInputSourceKind kind);

enum class WorldReactionOutputLocationPolicy {
    Unknown,
    ActorInventory,
    DropOnMap
};

std::string toString(WorldReactionOutputLocationPolicy policy);
pathfinder::foundation::Result<WorldReactionOutputLocationPolicy> worldReactionOutputLocationPolicyFromString(const std::string& str);

enum class WorldReactionFailureKind {
    None,
    InvalidRequest,
    ActorMissing,
    ReactionMissing,
    ReactionInvalid,
    ActionMismatch,
    InventoryMissing,
    InputMissing,
    InputStateMismatch,
    QuantityInsufficient,
    OutputInvalid,
    OutputPolicyInvalid,
    ConsumeFailed,
    SpawnFailed,
    RuntimeConflict
};

std::string toString(WorldReactionFailureKind kind);

// ---------------------------------------------------------------------------
// Core DTOs
// ---------------------------------------------------------------------------

struct WorldReactionRequest {
    std::string reaction_command_id;
    std::string actor_key;
    std::string reaction_key;
    WorldReactionActionKind action_kind = WorldReactionActionKind::Craft;
    WorldReactionOutputLocationPolicy output_location_policy = WorldReactionOutputLocationPolicy::ActorInventory;
    std::map<std::string, std::string> parameters;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldReactionInputProof {
    std::string object_key;
    std::string required_state;
    int required_quantity = 1;
    int available_quantity = 0;
    WorldReactionInputSourceKind source_kind = WorldReactionInputSourceKind::Unknown;
    std::string entity_id;
    std::string inventory_id;
    std::optional<world_runtime::WorldCellCoord> coord;
    std::vector<std::string> state_keys;
};

struct WorldReactionConsumeDraft {
    std::string object_key;
    std::string entity_id;
    int quantity = 1;
    WorldReactionInputSourceKind source_kind = WorldReactionInputSourceKind::Unknown;
    std::string inventory_id;
    std::optional<world_runtime::WorldCellCoord> coord;
};

struct WorldReactionOutputDraft {
    std::string entity_id;
    std::string entity_key;
    std::string display_name_key;
    int quantity = 1;
    bool stackable = true;
    std::string stack_key;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
    std::vector<std::string> tag_keys;
    WorldReactionOutputLocationPolicy location_policy = WorldReactionOutputLocationPolicy::ActorInventory;
    std::optional<world_runtime::WorldCellCoord> coord;
};

struct WorldReactionDraft {
    std::string draft_id;
    WorldReactionRequest request;
    content::ReactionDefinitionContent reaction;
    std::vector<WorldReactionInputProof> input_proofs;
    std::vector<WorldReactionConsumeDraft> consume_drafts;
    std::vector<WorldReactionOutputDraft> output_drafts;
    std::string effect_key;
    std::vector<std::string> knowledge_template_keys;
};

struct WorldReactionResult {
    bool ok = false;
    WorldReactionFailureKind failure_kind = WorldReactionFailureKind::None;
    std::vector<std::string> changed_inventory_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> changed_cell_ids;
    std::vector<world_command::WorldStateDeltaDto> state_deltas;
    std::vector<world_command::WorldEventDto> events;
    std::vector<world_command::WorldExperienceDto> experiences;
};

} // namespace pathfinder::world_reaction
