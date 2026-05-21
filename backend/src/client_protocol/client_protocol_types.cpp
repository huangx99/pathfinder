#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::client_protocol {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// ClientRequestKind
// ---------------------------------------------------------------------------
std::string toString(ClientRequestKind kind) {
    switch (kind) {
        case ClientRequestKind::Unknown:    return "unknown";
        case ClientRequestKind::Bootstrap:  return "bootstrap";
        case ClientRequestKind::Command:    return "command";
        case ClientRequestKind::Refresh:    return "refresh";
        case ClientRequestKind::Reset:      return "reset";
        case ClientRequestKind::TestOnly:   return "test_only";
        default: return "unknown";
    }
}

Result<ClientRequestKind> clientRequestKindFromString(const std::string& str) {
    if (str == "bootstrap")   return Result<ClientRequestKind>::ok(ClientRequestKind::Bootstrap);
    if (str == "command")     return Result<ClientRequestKind>::ok(ClientRequestKind::Command);
    if (str == "refresh")     return Result<ClientRequestKind>::ok(ClientRequestKind::Refresh);
    if (str == "reset")       return Result<ClientRequestKind>::ok(ClientRequestKind::Reset);
    if (str == "test_only")   return Result<ClientRequestKind>::ok(ClientRequestKind::TestOnly);
    return Result<ClientRequestKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown ClientRequestKind: " + str)
    );
}

// ---------------------------------------------------------------------------
// ClientSubmitMode
// ---------------------------------------------------------------------------
std::string toString(ClientSubmitMode mode) {
    switch (mode) {
        case ClientSubmitMode::Unknown:          return "unknown";
        case ClientSubmitMode::OptionId:         return "option_id";
        case ClientSubmitMode::WorldCommandDto:  return "world_command_dto";
        case ClientSubmitMode::RefreshOnly:      return "refresh_only";
        case ClientSubmitMode::TestOnly:         return "test_only";
        default: return "unknown";
    }
}

Result<ClientSubmitMode> clientSubmitModeFromString(const std::string& str) {
    if (str == "option_id")          return Result<ClientSubmitMode>::ok(ClientSubmitMode::OptionId);
    if (str == "world_command_dto")  return Result<ClientSubmitMode>::ok(ClientSubmitMode::WorldCommandDto);
    if (str == "refresh_only")       return Result<ClientSubmitMode>::ok(ClientSubmitMode::RefreshOnly);
    if (str == "test_only")          return Result<ClientSubmitMode>::ok(ClientSubmitMode::TestOnly);
    return Result<ClientSubmitMode>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown ClientSubmitMode: " + str)
    );
}

// ---------------------------------------------------------------------------
// ClientSyncState
// ---------------------------------------------------------------------------
std::string toString(ClientSyncState state) {
    switch (state) {
        case ClientSyncState::Unknown:           return "unknown";
        case ClientSyncState::InSync:            return "in_sync";
        case ClientSyncState::CommandPending:    return "command_pending";
        case ClientSyncState::PatchApplying:     return "patch_applying";
        case ClientSyncState::NeedsFullRefresh:  return "needs_full_refresh";
        case ClientSyncState::OutOfSync:         return "out_of_sync";
        case ClientSyncState::FatalError:        return "fatal_error";
        case ClientSyncState::TestOnly:          return "test_only";
        default: return "unknown";
    }
}

Result<ClientSyncState> clientSyncStateFromString(const std::string& str) {
    if (str == "in_sync")              return Result<ClientSyncState>::ok(ClientSyncState::InSync);
    if (str == "command_pending")      return Result<ClientSyncState>::ok(ClientSyncState::CommandPending);
    if (str == "patch_applying")       return Result<ClientSyncState>::ok(ClientSyncState::PatchApplying);
    if (str == "needs_full_refresh")   return Result<ClientSyncState>::ok(ClientSyncState::NeedsFullRefresh);
    if (str == "out_of_sync")          return Result<ClientSyncState>::ok(ClientSyncState::OutOfSync);
    if (str == "fatal_error")          return Result<ClientSyncState>::ok(ClientSyncState::FatalError);
    if (str == "test_only")            return Result<ClientSyncState>::ok(ClientSyncState::TestOnly);
    return Result<ClientSyncState>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown ClientSyncState: " + str)
    );
}

// ---------------------------------------------------------------------------
// ClientProjectionScope
// ---------------------------------------------------------------------------
std::string toString(ClientProjectionScope scope) {
    switch (scope) {
        case ClientProjectionScope::Unknown:         return "unknown";
        case ClientProjectionScope::Viewport:        return "viewport";
        case ClientProjectionScope::ActorSelf:       return "actor_self";
        case ClientProjectionScope::Inventory:       return "inventory";
        case ClientProjectionScope::Knowledge:       return "knowledge";
        case ClientProjectionScope::EventFeed:       return "event_feed";
        case ClientProjectionScope::AgentSummary:    return "agent_summary";
        case ClientProjectionScope::FullSafeWorld:   return "full_safe_world";
        case ClientProjectionScope::TestOnly:        return "test_only";
        default: return "unknown";
    }
}

Result<ClientProjectionScope> clientProjectionScopeFromString(const std::string& str) {
    if (str == "viewport")         return Result<ClientProjectionScope>::ok(ClientProjectionScope::Viewport);
    if (str == "actor_self")       return Result<ClientProjectionScope>::ok(ClientProjectionScope::ActorSelf);
    if (str == "inventory")        return Result<ClientProjectionScope>::ok(ClientProjectionScope::Inventory);
    if (str == "knowledge")        return Result<ClientProjectionScope>::ok(ClientProjectionScope::Knowledge);
    if (str == "event_feed")       return Result<ClientProjectionScope>::ok(ClientProjectionScope::EventFeed);
    if (str == "agent_summary")    return Result<ClientProjectionScope>::ok(ClientProjectionScope::AgentSummary);
    if (str == "full_safe_world")  return Result<ClientProjectionScope>::ok(ClientProjectionScope::FullSafeWorld);
    if (str == "test_only")        return Result<ClientProjectionScope>::ok(ClientProjectionScope::TestOnly);
    return Result<ClientProjectionScope>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown ClientProjectionScope: " + str)
    );
}

// ---------------------------------------------------------------------------
// DTO validateBasic implementations
// ---------------------------------------------------------------------------
Result<void> ClientSelectionContext::validateBasic() const {
    auto target_result = target.validateBasic();
    if (target_result.is_error()) {
        return target_result;
    }
    return Result<void>::ok();
}

Result<void> ClientWorldProjection::validateBasic() const {
    for (const auto& cell : visible_cells) {
        auto r = cell.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& entity : visible_entities) {
        auto r = entity.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& inv : inventories) {
        auto r = inv.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& kn : knowledge) {
        auto r = kn.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& ae : area_effects) {
        auto r = ae.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

Result<void> ClientBootstrapRequest::validateBasic() const {
    if (client_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientBootstrapRequest.client_id cannot be empty")
        );
    }
    if (session_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientBootstrapRequest.session_id cannot be empty")
        );
    }
    if (client_protocol_version == 0) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientBootstrapRequest.client_protocol_version cannot be 0")
        );
    }
    return Result<void>::ok();
}

Result<void> ClientBootstrapResponse::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientBootstrapResponse.session_id cannot be empty")
        );
    }
    if (server_protocol_version == 0) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientBootstrapResponse.server_protocol_version cannot be 0")
        );
    }
    auto proj_result = full_projection.validateBasic();
    if (proj_result.is_error()) {
        return proj_result;
    }
    for (const auto& opt : available_commands) {
        auto r = opt.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& evt : event_feed) {
        auto r = evt.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

Result<void> ClientCommandRequest::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientCommandRequest.session_id cannot be empty")
        );
    }
    if (client_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientCommandRequest.client_id cannot be empty")
        );
    }
    if (submit_mode == ClientSubmitMode::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientCommandRequest.submit_mode cannot be Unknown")
        );
    }
    if (submit_mode == ClientSubmitMode::OptionId && option_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientCommandRequest.option_id required when submit_mode is OptionId")
        );
    }
    if (submit_mode == ClientSubmitMode::WorldCommandDto && !command.has_value()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientCommandRequest.command required when submit_mode is WorldCommandDto")
        );
    }
    if (command.has_value()) {
        auto cmd_result = command->validateBasic();
        if (cmd_result.is_error()) {
            return cmd_result;
        }
    }
    auto ctx_result = selection_context.validateBasic();
    if (ctx_result.is_error()) {
        return ctx_result;
    }
    return Result<void>::ok();
}

Result<void> ClientCommandResponse::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientCommandResponse.session_id cannot be empty")
        );
    }
    auto result_validation = result.validateBasic();
    if (result_validation.is_error()) {
        return result_validation;
    }
    auto patch_validation = projection_patch.validateBasic();
    if (patch_validation.is_error()) {
        return patch_validation;
    }
    for (const auto& opt : available_commands) {
        auto r = opt.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& evt : event_feed) {
        auto r = evt.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& exp : experiences) {
        auto r = exp.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

Result<void> ClientRefreshRequest::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientRefreshRequest.session_id cannot be empty")
        );
    }
    if (client_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientRefreshRequest.client_id cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> ClientRefreshResponse::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientRefreshResponse.session_id cannot be empty")
        );
    }
    auto proj_result = full_projection.validateBasic();
    if (proj_result.is_error()) {
        return proj_result;
    }
    for (const auto& opt : available_commands) {
        auto r = opt.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& evt : event_feed) {
        auto r = evt.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

Result<void> ClientResetRequest::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientResetRequest.session_id cannot be empty")
        );
    }
    if (client_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientResetRequest.client_id cannot be empty")
        );
    }
    if (!confirmed) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientResetRequest.confirmed must be true")
        );
    }
    return Result<void>::ok();
}

Result<void> ClientResetResponse::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientResetResponse.session_id cannot be empty")
        );
    }
    auto boot_result = bootstrap.validateBasic();
    if (boot_result.is_error()) {
        return boot_result;
    }
    return Result<void>::ok();
}

Result<void> ClientOptionMaterializeRecord::validateBasic() const {
    if (option_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientOptionMaterializeRecord.option_id cannot be empty")
        );
    }
    if (command_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "ClientOptionMaterializeRecord.command_id cannot be empty")
        );
    }
    return Result<void>::ok();
}

} // namespace pathfinder::client_protocol
