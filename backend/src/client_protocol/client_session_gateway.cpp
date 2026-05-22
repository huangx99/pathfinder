#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::client_protocol {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::world_map_interaction::RegionActivityWindowService;
using pathfinder::world_map_interaction::RegionLifecycleTriggerService;
using pathfinder::world_map_interaction::ClientRegionLifecycleEventDto;

namespace {

// P60: Convert lifecycle events to WorldEventDto for client feed.
std::vector<WorldEventDto> convertLifecycleEvents(
    const std::vector<ClientRegionLifecycleEventDto>& lifecycle_events) {
    std::vector<WorldEventDto> result;
    for (const auto& ev : lifecycle_events) {
        WorldEventDto event;
        event.event_id = ev.event_id;
        event.event_kind = "Lifecycle:" + ev.event_kind;
        event.tick = ev.tick;
        event.title_text = ev.label_zh;
        event.body_text = ev.label_zh;
        for (const auto& r : ev.failure_reason_keys) {
            event.reason_keys.push_back(r);
        }
        result.push_back(std::move(event));
    }
    return result;
}

// P60: Run activity window + lifecycle trigger for an actor.
std::vector<WorldEventDto> runLifecycleTrigger(
    RegionActivityWindowService* aw_service,
    RegionLifecycleTriggerService* trigger_service,
    const std::string& actor_key,
    const std::string& layer_key,
    uint64_t tick,
    const world_region_state::WorldRegionSnapshotBuildContext& context) {
    if (!aw_service || !trigger_service) {
        return {};
    }
    auto window_res = aw_service->computeWindow(actor_key, layer_key, tick);
    if (window_res.is_error()) {
        return {};
    }
    auto trigger_result = trigger_service->processWindow(window_res.value(), context);
    return convertLifecycleEvents(trigger_result.events);
}

} // anonymous namespace

ClientSessionGateway::ClientSessionGateway(
    const ClientProjectionAdapter& projection_adapter,
    const ClientAvailableCommandAdapter& available_command_adapter)
    : projection_adapter_(projection_adapter)
    , available_command_adapter_(available_command_adapter) {}

void ClientSessionGateway::setRegionEnsureAdapter(
    client_runtime_bridge::ClientWorldRegionEnsureAdapter* ensure_adapter) {
    ensure_adapter_ = ensure_adapter;
}

void ClientSessionGateway::setActivityWindowService(
    world_map_interaction::RegionActivityWindowService* service) {
    activity_window_service_ = service;
}

void ClientSessionGateway::setLifecycleTriggerService(
    world_map_interaction::RegionLifecycleTriggerService* service) {
    lifecycle_trigger_service_ = service;
}

void ClientSessionGateway::setWorldContext(const std::string& world_id, uint64_t world_seed) {
    world_id_ = world_id;
    world_seed_ = world_seed;
}

ClientSessionGateway::SessionState& ClientSessionGateway::getOrCreateSession(
    const ClientBootstrapRequest& request) {
    auto it = sessions_.find(request.session_id);
    if (it != sessions_.end()) {
        return it->second;
    }
    SessionState state;
    state.session_id = request.session_id;
    state.client_id = request.client_id;
    state.actor_key = request.requested_actor_key.empty() ? "player" : request.requested_actor_key;
    state.layer_key = request.requested_layer_key.empty() ? "surface" : request.requested_layer_key;
    state.projection_version = 1;
    auto [inserted_it, _] = sessions_.emplace(request.session_id, std::move(state));
    return inserted_it->second;
}

Result<ClientBootstrapResponse> ClientSessionGateway::bootstrap(
    const ClientBootstrapRequest& request) {
    auto valid = request.validateBasic();
    if (valid.is_error()) {
        return Result<ClientBootstrapResponse>::fail(valid.errors());
    }

    // Prevent session hijacking: if session exists with different client_id, reject.
    auto it = sessions_.find(request.session_id);
    if (it != sessions_.end() && it->second.client_id != request.client_id) {
        return Result<ClientBootstrapResponse>::fail(
            makeError(ErrorCode::command_not_allowed_now, "client_id mismatch")
        );
    }

    auto& session = getOrCreateSession(request);

    // P58/P60: Ensure and lifecycle must run before projection/options are built.
    // Otherwise the client may receive cells/options from regions that are sealed
    // and detached immediately afterwards, causing pathfinding to act on stale map.
    if (ensure_adapter_) {
        auto ensure_res = ensure_adapter_->ensureForBootstrap(session.actor_key, session.layer_key);
        if (ensure_res.is_error()) {
            return Result<ClientBootstrapResponse>::fail(ensure_res.errors());
        }
        auto ensure_cmd_res = ensure_adapter_->ensureForAvailableCommands(session.actor_key, session.layer_key);
        if (ensure_cmd_res.is_error()) {
            return Result<ClientBootstrapResponse>::fail(ensure_cmd_res.errors());
        }
    }

    auto lifecycle_tick_res = available_command_adapter_.runtimeStateVersion(session.actor_key, session.layer_key);
    uint64_t lifecycle_tick = lifecycle_tick_res.is_ok() && lifecycle_tick_res.value() > 0
        ? lifecycle_tick_res.value()
        : session.projection_version;

    world_region_state::WorldRegionSnapshotBuildContext ctx;
    ctx.world_id = world_id_;
    ctx.world_seed = world_seed_;
    ctx.world_tick = lifecycle_tick;
    ctx.state_version = lifecycle_tick;
    ctx.worldgen_profile_key = "first_world";
    ctx.generator_version = "1.0.0";
    ctx.content_version = "1.0.0";
    auto lifecycle_events = runLifecycleTrigger(
        activity_window_service_, lifecycle_trigger_service_,
        session.actor_key, session.layer_key, lifecycle_tick, ctx);

    auto proj_result = projection_adapter_.buildFullProjection(
        session.actor_key, session.layer_key, session.projection_version);
    if (proj_result.is_error()) {
        return Result<ClientBootstrapResponse>::fail(proj_result.errors());
    }

    // P56: Sync session projection_version to runtime-backed full_projection version.
    const uint64_t runtime_version = proj_result.value().projection_version;
    session.projection_version = runtime_version;

    auto options_result = available_command_adapter_.buildOptions(
        session.actor_key, session.layer_key);
    if (options_result.is_error()) {
        return Result<ClientBootstrapResponse>::fail(options_result.errors());
    }

    // Store the authoritative option snapshot in session.
    session.last_available_commands = options_result.value();

    ClientBootstrapResponse response;
    response.session_id = session.session_id;
    response.server_protocol_version = 1;
    response.projection_version = runtime_version;
    response.full_projection = proj_result.value();
    response.available_commands = options_result.value();
    response.event_feed.insert(
        response.event_feed.end(), lifecycle_events.begin(), lifecycle_events.end());

    return Result<ClientBootstrapResponse>::ok(std::move(response));
}

Result<ClientRefreshResponse> ClientSessionGateway::refresh(
    const ClientRefreshRequest& request) {
    auto valid = request.validateBasic();
    if (valid.is_error()) {
        return Result<ClientRefreshResponse>::fail(valid.errors());
    }

    auto it = sessions_.find(request.session_id);
    if (it == sessions_.end()) {
        return Result<ClientRefreshResponse>::fail(
            makeError(ErrorCode::id_not_found, "Session not found: " + request.session_id)
        );
    }

    if (request.client_id != it->second.client_id) {
        return Result<ClientRefreshResponse>::fail(
            makeError(ErrorCode::command_not_allowed_now, "client_id mismatch")
        );
    }

    auto& session = it->second;

    // P58/P60: Ensure and lifecycle must run before refresh projection/options.
    if (ensure_adapter_) {
        auto ensure_res = ensure_adapter_->ensureForRefresh(session.actor_key, session.layer_key, 0);
        if (ensure_res.is_error()) {
            return Result<ClientRefreshResponse>::fail(ensure_res.errors());
        }
        auto ensure_cmd_res = ensure_adapter_->ensureForAvailableCommands(session.actor_key, session.layer_key);
        if (ensure_cmd_res.is_error()) {
            return Result<ClientRefreshResponse>::fail(ensure_cmd_res.errors());
        }
    }

    auto lifecycle_tick_res = available_command_adapter_.runtimeStateVersion(session.actor_key, session.layer_key);
    uint64_t lifecycle_tick = lifecycle_tick_res.is_ok() && lifecycle_tick_res.value() > 0
        ? lifecycle_tick_res.value()
        : session.projection_version;

    world_region_state::WorldRegionSnapshotBuildContext ctx;
    ctx.world_id = world_id_;
    ctx.world_seed = world_seed_;
    ctx.world_tick = lifecycle_tick;
    ctx.state_version = lifecycle_tick;
    ctx.worldgen_profile_key = "first_world";
    ctx.generator_version = "1.0.0";
    ctx.content_version = "1.0.0";
    auto lifecycle_events = runLifecycleTrigger(
        activity_window_service_, lifecycle_trigger_service_,
        session.actor_key, session.layer_key, lifecycle_tick, ctx);

    auto proj_result = projection_adapter_.buildScopedProjection(
        session.actor_key, session.layer_key, session.projection_version, request.requested_scopes);
    if (proj_result.is_error()) {
        return Result<ClientRefreshResponse>::fail(proj_result.errors());
    }

    // P56: Sync session projection_version to runtime-backed full_projection version.
    const uint64_t runtime_version = proj_result.value().projection_version;
    session.projection_version = runtime_version;

    auto options_result = available_command_adapter_.buildOptions(
        session.actor_key, session.layer_key);
    if (options_result.is_error()) {
        return Result<ClientRefreshResponse>::fail(options_result.errors());
    }

    // Update the authoritative option snapshot.
    session.last_available_commands = options_result.value();

    ClientRefreshResponse response;
    response.session_id = session.session_id;
    response.projection_version = runtime_version;
    response.full_projection = proj_result.value();
    response.available_commands = options_result.value();
    response.event_feed.insert(
        response.event_feed.end(), lifecycle_events.begin(), lifecycle_events.end());

    return Result<ClientRefreshResponse>::ok(std::move(response));
}

Result<ClientResetResponse> ClientSessionGateway::reset(
    const ClientResetRequest& request) {
    auto valid = request.validateBasic();
    if (valid.is_error()) {
        return Result<ClientResetResponse>::fail(valid.errors());
    }

    // Validate client_id before reset.
    auto it = sessions_.find(request.session_id);
    if (it != sessions_.end() && it->second.client_id != request.client_id) {
        return Result<ClientResetResponse>::fail(
            makeError(ErrorCode::command_not_allowed_now, "client_id mismatch")
        );
    }

    // Remove old session and create a new one.
    sessions_.erase(request.session_id);

    ClientBootstrapRequest bootstrap_request;
    bootstrap_request.client_id = request.client_id;
    bootstrap_request.session_id = request.session_id;
    bootstrap_request.create_if_missing = true;

    auto boot_result = bootstrap(bootstrap_request);
    if (boot_result.is_error()) {
        return Result<ClientResetResponse>::fail(boot_result.errors());
    }

    ClientResetResponse response;
    response.session_id = request.session_id;
    response.bootstrap = boot_result.value();
    return Result<ClientResetResponse>::ok(std::move(response));
}

Result<uint64_t> ClientSessionGateway::getProjectionVersion(
    const std::string& session_id) const {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return Result<uint64_t>::fail(
            makeError(ErrorCode::id_not_found, "Session not found: " + session_id)
        );
    }
    return Result<uint64_t>::ok(it->second.projection_version);
}

Result<std::string> ClientSessionGateway::getSessionActorKey(
    const std::string& session_id) const {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return Result<std::string>::fail(
            makeError(ErrorCode::id_not_found, "Session not found: " + session_id)
        );
    }
    return Result<std::string>::ok(it->second.actor_key);
}

Result<std::string> ClientSessionGateway::getSessionClientId(
    const std::string& session_id) const {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return Result<std::string>::fail(
            makeError(ErrorCode::id_not_found, "Session not found: " + session_id)
        );
    }
    return Result<std::string>::ok(it->second.client_id);
}

Result<std::string> ClientSessionGateway::getSessionLayerKey(
    const std::string& session_id) const {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return Result<std::string>::fail(
            makeError(ErrorCode::id_not_found, "Session not found: " + session_id)
        );
    }
    return Result<std::string>::ok(it->second.layer_key);
}

bool ClientSessionGateway::hasSession(const std::string& session_id) const {
    return sessions_.find(session_id) != sessions_.end();
}

Result<void> ClientSessionGateway::updateProjectionVersion(
    const std::string& session_id, uint64_t new_version) {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "Session not found: " + session_id)
        );
    }
    it->second.projection_version = new_version;
    return Result<void>::ok();
}

Result<std::vector<WorldCommandOptionDto>> ClientSessionGateway::getAvailableCommands(
    const std::string& session_id) const {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return Result<std::vector<WorldCommandOptionDto>>::fail(
            makeError(ErrorCode::id_not_found, "Session not found: " + session_id)
        );
    }
    return Result<std::vector<WorldCommandOptionDto>>::ok(it->second.last_available_commands);
}

bool ClientSessionGateway::isOptionValid(
    const std::string& session_id, const std::string& option_id) const {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return false;
    }
    for (const auto& opt : it->second.last_available_commands) {
        if (opt.option_id == option_id) {
            return true;
        }
    }
    return false;
}

Result<void> ClientSessionGateway::updateAvailableCommands(
    const std::string& session_id,
    const std::vector<WorldCommandOptionDto>& options) {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "Session not found: " + session_id)
        );
    }
    it->second.last_available_commands = options;
    return Result<void>::ok();
}

} // namespace pathfinder::client_protocol
