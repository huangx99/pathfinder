#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/world_command/world_command_types.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::client_protocol {

using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandOptionDto;
using pathfinder::world_command::WorldCommandResultDto;
using pathfinder::world_command::WorldCommandTargetDto;
using pathfinder::world_command::WorldEventDto;
using pathfinder::world_command::WorldExperienceDto;
using pathfinder::world_command::WorldFrontendHintDto;
using pathfinder::world_command::WorldProjectionPatchDto;
using pathfinder::world_command::WorldCellPatchDto;
using pathfinder::world_command::WorldEntityPatchDto;
using pathfinder::world_command::InventoryPatchDto;
using pathfinder::world_command::KnowledgePatchDto;
using pathfinder::world_command::AreaEffectPatchDto;
using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandSource;
using pathfinder::world_command::WorldCommandTargetKind;
using pathfinder::world_command::WorldCoordinateDto;
using pathfinder::world_command::WorldCommandContextDto;
using pathfinder::world_command::WorldStateDeltaDto;
using pathfinder::world_command::PatchOp;
using pathfinder::world_command::FrontendHintKind;
using pathfinder::world_command::AreaShapeKind;

// ---------------------------------------------------------------------------
// 7. Core Enums
// ---------------------------------------------------------------------------

enum class ClientRequestKind {
    Unknown,
    Bootstrap,
    Command,
    Refresh,
    Reset,
    TestOnly
};

std::string toString(ClientRequestKind kind);
pathfinder::foundation::Result<ClientRequestKind> clientRequestKindFromString(const std::string& str);

enum class ClientSubmitMode {
    Unknown,
    OptionId,
    WorldCommandDto,
    RefreshOnly,
    TestOnly
};

std::string toString(ClientSubmitMode mode);
pathfinder::foundation::Result<ClientSubmitMode> clientSubmitModeFromString(const std::string& str);

enum class ClientSyncState {
    Unknown,
    InSync,
    CommandPending,
    PatchApplying,
    NeedsFullRefresh,
    OutOfSync,
    FatalError,
    TestOnly
};

std::string toString(ClientSyncState state);
pathfinder::foundation::Result<ClientSyncState> clientSyncStateFromString(const std::string& str);

enum class ClientProjectionScope {
    Unknown,
    Viewport,
    ActorSelf,
    Inventory,
    Knowledge,
    EventFeed,
    AgentSummary,
    FullSafeWorld,
    TestOnly
};

std::string toString(ClientProjectionScope scope);
pathfinder::foundation::Result<ClientProjectionScope> clientProjectionScopeFromString(const std::string& str);

// ---------------------------------------------------------------------------
// 8. Core DTOs
// ---------------------------------------------------------------------------

struct ClientSelectionContext {
    WorldCommandTargetDto target;
    std::string selected_actor_key;
    std::string selected_entity_id;
    std::string selected_inventory_id;
    std::string selected_area_id;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientWorldProjection {
    uint64_t projection_version = 0;
    std::string actor_key;
    std::string active_layer_key;
    std::vector<WorldCellPatchDto> visible_cells;
    std::vector<WorldEntityPatchDto> visible_entities;
    std::vector<InventoryPatchDto> inventories;
    std::vector<KnowledgePatchDto> knowledge;
    std::vector<AreaEffectPatchDto> area_effects;
    std::vector<std::string> safe_summary_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientBootstrapRequest {
    std::string client_id;
    std::string session_id;
    std::string requested_actor_key;
    std::string requested_layer_key;
    uint64_t client_protocol_version = 1;
    bool create_if_missing = true;
    bool dev_reset_if_allowed = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientBootstrapResponse {
    std::string session_id;
    uint64_t server_protocol_version = 1;
    uint64_t projection_version = 0;
    ClientWorldProjection full_projection;
    std::vector<WorldCommandOptionDto> available_commands;
    std::vector<WorldEventDto> event_feed;
    std::vector<WorldFrontendHintDto> frontend_hints;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientCommandRequest {
    std::string session_id;
    std::string client_id;
    uint64_t client_sequence = 0;
    uint64_t known_projection_version = 0;
    ClientSubmitMode submit_mode = ClientSubmitMode::Unknown;
    std::string option_id;
    std::optional<WorldCommandDto> command;
    ClientSelectionContext selection_context;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientCommandResponse {
    std::string session_id;
    uint64_t accepted_client_sequence = 0;
    uint64_t base_projection_version = 0;
    uint64_t new_projection_version = 0;
    bool requires_full_refresh = false;
    WorldCommandResultDto result;
    WorldProjectionPatchDto projection_patch;
    std::vector<WorldCommandOptionDto> available_commands;
    std::vector<WorldEventDto> event_feed;
    std::vector<WorldExperienceDto> experiences;
    std::vector<WorldFrontendHintDto> frontend_hints;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientRefreshRequest {
    std::string session_id;
    std::string client_id;
    uint64_t known_projection_version = 0;
    std::vector<ClientProjectionScope> requested_scopes;
    std::string requested_layer_key;
    int viewport_center_x = 0;
    int viewport_center_y = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientRefreshResponse {
    std::string session_id;
    uint64_t projection_version = 0;
    ClientWorldProjection full_projection;
    std::vector<WorldCommandOptionDto> available_commands;
    std::vector<WorldEventDto> event_feed;
    std::vector<WorldFrontendHintDto> frontend_hints;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientResetRequest {
    std::string session_id;
    std::string client_id;
    bool confirmed = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientResetResponse {
    std::string session_id;
    ClientBootstrapResponse bootstrap;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 16. Option materialization record (for traceability)
// ---------------------------------------------------------------------------

struct ClientOptionMaterializeRecord {
    std::string option_id;
    std::string command_id;
    WorldCommandKind command_kind = WorldCommandKind::Unknown;
    std::string materialize_reason_key;
    bool success = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::client_protocol
