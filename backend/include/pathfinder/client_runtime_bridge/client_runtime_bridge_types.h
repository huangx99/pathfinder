#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/foundation/result.h"
#include <cstdint>
#include <string>
#include <vector>

namespace pathfinder::client_runtime_bridge {

// ---------------------------------------------------------------------------
// 9. Core Enums
// ---------------------------------------------------------------------------

enum class ClientRuntimeBridgeMode {
    Unknown,
    RuntimeBacked,
    StubForProtocolTest,
    Disabled,
    TestOnly
};

std::string toString(ClientRuntimeBridgeMode mode);
pathfinder::foundation::Result<ClientRuntimeBridgeMode> clientRuntimeBridgeModeFromString(const std::string& str);

enum class ClientCommandOptionProviderKind {
    Unknown,
    Wait,
    Move,
    Inspect,
    Inventory,
    Harvest,
    Craft,
    Teach,
    Combat,
    AreaEffect,
    TestOnly
};

std::string toString(ClientCommandOptionProviderKind kind);
pathfinder::foundation::Result<ClientCommandOptionProviderKind> clientCommandOptionProviderKindFromString(const std::string& str);

enum class ClientMoveDirection {
    Unknown,
    North,
    South,
    West,
    East,
    TestOnly
};

std::string toString(ClientMoveDirection direction);
pathfinder::foundation::Result<ClientMoveDirection> clientMoveDirectionFromString(const std::string& str);
int dxForDirection(ClientMoveDirection direction);
int dyForDirection(ClientMoveDirection direction);

enum class ClientProjectionBuildReason {
    Unknown,
    Bootstrap,
    Refresh,
    CommandResult,
    Reset,
    TestOnly
};

std::string toString(ClientProjectionBuildReason reason);
pathfinder::foundation::Result<ClientProjectionBuildReason> clientProjectionBuildReasonFromString(const std::string& str);

// ---------------------------------------------------------------------------
// 10. Core DTOs / Data Structures
// ---------------------------------------------------------------------------

struct ClientRuntimeViewRequest {
    std::string actor_key;
    std::string layer_key;
    uint64_t projection_version_hint = 0;
    std::vector<pathfinder::client_protocol::ClientProjectionScope> scopes;
    int viewport_center_x = 0;
    int viewport_center_y = 0;
    ClientProjectionBuildReason reason = ClientProjectionBuildReason::Unknown;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientRuntimeView {
    uint64_t projection_version = 0;
    std::string actor_key;
    std::string active_layer_key;
    std::vector<pathfinder::world_command::WorldCellPatchDto> visible_cells;
    std::vector<pathfinder::world_command::WorldEntityPatchDto> visible_entities;
    std::vector<pathfinder::world_command::InventoryPatchDto> inventories;
    std::vector<pathfinder::world_command::KnowledgePatchDto> knowledge;
    std::vector<pathfinder::world_command::AreaEffectPatchDto> area_effects;
    std::vector<std::string> safe_summary_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientRuntimeCommandOptionRequest {
    std::string actor_key;
    std::string layer_key;
    uint64_t projection_version = 0;
    bool include_disabled = true;
    std::vector<ClientCommandOptionProviderKind> provider_kinds;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientRuntimeBridgeDiagnostics {
    ClientRuntimeBridgeMode bridge_mode = ClientRuntimeBridgeMode::Unknown;
    uint64_t runtime_state_version = 0;
    int visible_cell_count = 0;
    int visible_entity_count = 0;
    int option_count = 0;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::client_runtime_bridge
