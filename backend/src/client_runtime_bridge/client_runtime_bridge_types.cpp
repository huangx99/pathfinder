#include "pathfinder/client_runtime_bridge/client_runtime_bridge_types.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <set>

namespace pathfinder::client_runtime_bridge {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// ClientRuntimeBridgeMode
// ---------------------------------------------------------------------------

std::string toString(ClientRuntimeBridgeMode mode) {
    switch (mode) {
        case ClientRuntimeBridgeMode::RuntimeBacked:     return "runtime_backed";
        case ClientRuntimeBridgeMode::StubForProtocolTest: return "stub_for_protocol_test";
        case ClientRuntimeBridgeMode::Disabled:          return "disabled";
        case ClientRuntimeBridgeMode::TestOnly:          return "test_only";
        case ClientRuntimeBridgeMode::Unknown:
        default:                                         return "unknown";
    }
}

Result<ClientRuntimeBridgeMode> clientRuntimeBridgeModeFromString(const std::string& str) {
    if (str == "runtime_backed")             return Result<ClientRuntimeBridgeMode>::ok(ClientRuntimeBridgeMode::RuntimeBacked);
    if (str == "stub_for_protocol_test")     return Result<ClientRuntimeBridgeMode>::ok(ClientRuntimeBridgeMode::StubForProtocolTest);
    if (str == "disabled")                   return Result<ClientRuntimeBridgeMode>::ok(ClientRuntimeBridgeMode::Disabled);
    if (str == "test_only")                  return Result<ClientRuntimeBridgeMode>::ok(ClientRuntimeBridgeMode::TestOnly);
    return Result<ClientRuntimeBridgeMode>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown ClientRuntimeBridgeMode: " + str));
}

// ---------------------------------------------------------------------------
// ClientCommandOptionProviderKind
// ---------------------------------------------------------------------------

std::string toString(ClientCommandOptionProviderKind kind) {
    switch (kind) {
        case ClientCommandOptionProviderKind::Wait:        return "wait";
        case ClientCommandOptionProviderKind::Move:        return "move";
        case ClientCommandOptionProviderKind::Inspect:     return "inspect";
        case ClientCommandOptionProviderKind::Inventory:   return "inventory";
        case ClientCommandOptionProviderKind::Harvest:     return "harvest";
        case ClientCommandOptionProviderKind::Craft:       return "craft";
        case ClientCommandOptionProviderKind::Teach:       return "teach";
        case ClientCommandOptionProviderKind::Combat:      return "combat";
        case ClientCommandOptionProviderKind::AreaEffect:  return "area_effect";
        case ClientCommandOptionProviderKind::TestOnly:    return "test_only";
        case ClientCommandOptionProviderKind::Unknown:
        default:                                           return "unknown";
    }
}

Result<ClientCommandOptionProviderKind> clientCommandOptionProviderKindFromString(const std::string& str) {
    if (str == "wait")        return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::Wait);
    if (str == "move")        return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::Move);
    if (str == "inspect")     return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::Inspect);
    if (str == "inventory")   return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::Inventory);
    if (str == "harvest")     return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::Harvest);
    if (str == "craft")       return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::Craft);
    if (str == "teach")       return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::Teach);
    if (str == "combat")      return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::Combat);
    if (str == "area_effect") return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::AreaEffect);
    if (str == "test_only")   return Result<ClientCommandOptionProviderKind>::ok(ClientCommandOptionProviderKind::TestOnly);
    return Result<ClientCommandOptionProviderKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown ClientCommandOptionProviderKind: " + str));
}

// ---------------------------------------------------------------------------
// ClientMoveDirection
// ---------------------------------------------------------------------------

std::string toString(ClientMoveDirection direction) {
    switch (direction) {
        case ClientMoveDirection::North:     return "north";
        case ClientMoveDirection::South:     return "south";
        case ClientMoveDirection::West:      return "west";
        case ClientMoveDirection::East:      return "east";
        case ClientMoveDirection::TestOnly:  return "test_only";
        case ClientMoveDirection::Unknown:
        default:                             return "unknown";
    }
}

Result<ClientMoveDirection> clientMoveDirectionFromString(const std::string& str) {
    if (str == "north")       return Result<ClientMoveDirection>::ok(ClientMoveDirection::North);
    if (str == "south")       return Result<ClientMoveDirection>::ok(ClientMoveDirection::South);
    if (str == "west")        return Result<ClientMoveDirection>::ok(ClientMoveDirection::West);
    if (str == "east")        return Result<ClientMoveDirection>::ok(ClientMoveDirection::East);
    if (str == "test_only")   return Result<ClientMoveDirection>::ok(ClientMoveDirection::TestOnly);
    return Result<ClientMoveDirection>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown ClientMoveDirection: " + str));
}

int dxForDirection(ClientMoveDirection direction) {
    switch (direction) {
        case ClientMoveDirection::West:  return -1;
        case ClientMoveDirection::East:  return 1;
        default:                         return 0;
    }
}

int dyForDirection(ClientMoveDirection direction) {
    switch (direction) {
        case ClientMoveDirection::North: return -1;
        case ClientMoveDirection::South: return 1;
        default:                         return 0;
    }
}

// ---------------------------------------------------------------------------
// ClientProjectionBuildReason
// ---------------------------------------------------------------------------

std::string toString(ClientProjectionBuildReason reason) {
    switch (reason) {
        case ClientProjectionBuildReason::Bootstrap:      return "bootstrap";
        case ClientProjectionBuildReason::Refresh:        return "refresh";
        case ClientProjectionBuildReason::CommandResult:  return "command_result";
        case ClientProjectionBuildReason::Reset:          return "reset";
        case ClientProjectionBuildReason::TestOnly:       return "test_only";
        case ClientProjectionBuildReason::Unknown:
        default:                                          return "unknown";
    }
}

Result<ClientProjectionBuildReason> clientProjectionBuildReasonFromString(const std::string& str) {
    if (str == "bootstrap")      return Result<ClientProjectionBuildReason>::ok(ClientProjectionBuildReason::Bootstrap);
    if (str == "refresh")        return Result<ClientProjectionBuildReason>::ok(ClientProjectionBuildReason::Refresh);
    if (str == "command_result") return Result<ClientProjectionBuildReason>::ok(ClientProjectionBuildReason::CommandResult);
    if (str == "reset")          return Result<ClientProjectionBuildReason>::ok(ClientProjectionBuildReason::Reset);
    if (str == "test_only")      return Result<ClientProjectionBuildReason>::ok(ClientProjectionBuildReason::TestOnly);
    return Result<ClientProjectionBuildReason>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown ClientProjectionBuildReason: " + str));
}

// ---------------------------------------------------------------------------
// DTO validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientRuntimeViewRequest::validateBasic() const {
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "actor_key is required"));
    }
    if (layer_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "layer_key is required"));
    }
    if (reason == ClientProjectionBuildReason::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "reason cannot be Unknown"));
    }
    for (const auto& scope : scopes) {
        if (scope == pathfinder::client_protocol::ClientProjectionScope::Unknown) {
            return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "scope cannot be Unknown"));
        }
    }
    return Result<void>::ok();
}

Result<void> ClientRuntimeView::validateBasic() const {
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "actor_key is required"));
    }
    if (active_layer_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "active_layer_key is required"));
    }
    std::set<std::string> seen_cell_ids;
    for (const auto& cell : visible_cells) {
        auto cell_valid = cell.validateBasic();
        if (cell_valid.is_error()) return cell_valid;
        if (!seen_cell_ids.insert(cell.cell_id).second) {
            return Result<void>::fail(makeError(ErrorCode::command_duplicate, "duplicate cell_id: " + cell.cell_id));
        }
    }
    std::set<std::string> seen_entity_ids;
    for (const auto& entity : visible_entities) {
        auto entity_valid = entity.validateBasic();
        if (entity_valid.is_error()) return entity_valid;
        if (!seen_entity_ids.insert(entity.entity_id).second) {
            return Result<void>::fail(makeError(ErrorCode::command_duplicate, "duplicate entity_id: " + entity.entity_id));
        }
    }
    return Result<void>::ok();
}

Result<void> ClientRuntimeCommandOptionRequest::validateBasic() const {
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "actor_key is required"));
    }
    if (layer_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "layer_key is required"));
    }
    for (const auto& kind : provider_kinds) {
        if (kind == ClientCommandOptionProviderKind::Unknown) {
            return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "provider_kind cannot be Unknown"));
        }
    }
    return Result<void>::ok();
}

Result<void> ClientRuntimeBridgeDiagnostics::validateBasic() const {
    if (bridge_mode == ClientRuntimeBridgeMode::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "bridge_mode cannot be Unknown"));
    }
    return Result<void>::ok();
}

} // namespace pathfinder::client_runtime_bridge
