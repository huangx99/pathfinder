#include "pathfinder/client_protocol/client_command_gateway.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/logging/logger.h"
#include <optional>
#include <sstream>

namespace pathfinder::client_protocol {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::world_command::WorldCommandResponseDto;
using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandResultKind;
using pathfinder::world_command::WorldCommandSource;

namespace {

std::string joinReasons(const std::vector<std::string>& reasons) {
    if (reasons.empty()) return "none";
    std::ostringstream oss;
    for (size_t i = 0; i < reasons.size(); ++i) {
        if (i > 0) oss << ',';
        oss << reasons[i];
    }
    return oss.str();
}

std::string targetSummary(const WorldCommandTargetDto& target) {
    std::ostringstream oss;
    oss << "kind=" << pathfinder::world_command::toString(target.target_kind);
    if (target.target_coord) {
        oss << " coord=" << target.target_coord->x << ',' << target.target_coord->y
            << ',' << target.target_coord->layer_key;
    }
    if (!target.target_entity_id.empty()) oss << " entity=" << target.target_entity_id;
    if (!target.target_actor_key.empty()) oss << " target_actor=" << target.target_actor_key;
    if (!target.target_item_key.empty()) oss << " item=" << target.target_item_key;
    if (!target.target_inventory_id.empty()) oss << " inventory=" << target.target_inventory_id;
    return oss.str();
}

void logBlockedClientCommand(
    const std::string& session_id,
    uint64_t client_sequence,
    uint64_t projection_version,
    const std::string& actor_key,
    const std::string& reason_key) {
    std::ostringstream oss;
    oss << "blocked session=" << session_id
        << " seq=" << client_sequence
        << " actor=" << actor_key
        << " projection=" << projection_version
        << " reason=" << reason_key;
    pathfinder::logging::log(pathfinder::logging::tag::Command, oss.str());
}

} // namespace

ClientCommandGateway::ClientCommandGateway(
    ClientSessionGateway& session_gateway,
    pathfinder::world_command::WorldCommandPipeline& pipeline,
    const ClientPatchContract& patch_contract,
    const ClientAvailableCommandAdapter& available_command_adapter)
    : session_gateway_(session_gateway)
    , pipeline_(pipeline)
    , patch_contract_(patch_contract)
    , available_command_adapter_(available_command_adapter) {}

void ClientCommandGateway::setRegionEnsureAdapter(
    client_runtime_bridge::ClientWorldRegionEnsureAdapter* ensure_adapter) {
    ensure_adapter_ = ensure_adapter;
}

void ClientCommandGateway::setActivityWindowService(
    world_map_interaction::RegionActivityWindowService* service) {
    activity_window_service_ = service;
}

void ClientCommandGateway::setLifecycleTriggerService(
    world_map_interaction::RegionLifecycleTriggerService* service) {
    lifecycle_trigger_service_ = service;
}

void ClientCommandGateway::setWorldContext(const std::string& world_id, uint64_t world_seed) {
    world_id_ = world_id;
    world_seed_ = world_seed;
}

void ClientCommandGateway::setPostCommandHook(PostCommandHook hook) {
    post_command_hook_ = std::move(hook);
}

Result<void> ClientCommandGateway::validateClientCommandDto(
    const WorldCommandDto& command,
    const std::string& session_actor_key,
    const std::vector<WorldCommandOptionDto>& snapshot) const {

    // Actor must match session-bound actor.
    if (command.actor_key != session_actor_key) {
        return Result<void>::fail(
            makeError(ErrorCode::command_not_allowed_now,
                "Command actor_key does not match session actor: " + command.actor_key)
        );
    }

    // Source must be PlayerInput for client-submitted commands.
    if (command.source != WorldCommandSource::PlayerInput) {
        return Result<void>::fail(
            makeError(ErrorCode::command_not_allowed_now,
                "Client command source must be PlayerInput")
        );
    }

    // Reject hidden debug flag.
    if (command.context.allow_hidden_debug) {
        return Result<void>::fail(
            makeError(ErrorCode::command_not_allowed_now,
                "Client commands cannot use allow_hidden_debug")
        );
    }

    // Reject unsafe parameters that could bypass client protocol constraints.
    static const std::vector<std::string> unsafe_param_keys = {
        "recipient_claim_json",
        "knowledge_json",
        "force_learn",
        "force_teach",
        "learned",
        "raw_state",
        "hidden_truth",
        "content_key_rule_hint"
    };
    for (const auto& key : unsafe_param_keys) {
        if (command.parameters.find(key) != command.parameters.end()) {
            return Result<void>::fail(
                makeError(ErrorCode::command_not_allowed_now,
                    "Unsafe parameter not allowed: " + key)
            );
        }
    }

    // Client-submitted WorldCommandDto.parameters must be empty.
    // available_commands does not currently express parameter signatures,
    // so any parameters are treated as unissued / forged.
    if (!command.parameters.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_not_allowed_now,
                "Client WorldCommandDto parameters must be empty")
        );
    }

    // WorldCommandDto must match an option in the latest available_commands snapshot.
    // This prevents clients from bypassing option authority and executing unissued commands.
    auto targetsEqual = [](const WorldCommandTargetDto& a, const WorldCommandTargetDto& b) -> bool {
        if (a.target_kind != b.target_kind) return false;
        if (a.target_entity_id != b.target_entity_id) return false;
        if (a.target_actor_key != b.target_actor_key) return false;
        if (a.target_item_key != b.target_item_key) return false;
        if (a.target_inventory_id != b.target_inventory_id) return false;
        if (a.target_area_id != b.target_area_id) return false;
        if (a.knowledge_key != b.knowledge_key) return false;
        if (a.recipe_key != b.recipe_key) return false;
        if (a.target_coord.has_value() != b.target_coord.has_value()) return false;
        if (a.target_coord.has_value()) {
            if (a.target_coord->x != b.target_coord->x) return false;
            if (a.target_coord->y != b.target_coord->y) return false;
            if (a.target_coord->layer_key != b.target_coord->layer_key) return false;
        }
        return true;
    };

    bool matched = false;
    for (const auto& opt : snapshot) {
        if (opt.command_kind == command.command_kind &&
            opt.command_key == command.command_key &&
            targetsEqual(opt.target, command.target)) {
            matched = true;
            break;
        }
    }
    if (!matched) {
        return Result<void>::fail(
            makeError(ErrorCode::command_not_allowed_now,
                "Command kind/key/target does not match any available option")
        );
    }

    return Result<void>::ok();
}

Result<WorldCommandDto> ClientCommandGateway::resolveCommand(
    const ClientCommandRequest& request,
    const std::string& actor_key) {

    if (request.submit_mode == ClientSubmitMode::OptionId) {
        // Authority: option_id must exist in the session's latest snapshot.
        if (!session_gateway_.isOptionValid(request.session_id, request.option_id)) {
            return Result<WorldCommandDto>::fail(
                makeError(ErrorCode::command_unknown_type,
                    "Unknown or expired option_id: " + request.option_id)
            );
        }

        auto snapshot_result = session_gateway_.getAvailableCommands(request.session_id);
        if (snapshot_result.is_error()) {
            return Result<WorldCommandDto>::fail(snapshot_result.errors());
        }

        auto result = available_command_adapter_.materializeOption(
            request.option_id, actor_key, snapshot_result.value());
        if (result.is_error()) {
            return Result<WorldCommandDto>::fail(result.errors());
        }
        auto cmd = result.value();
        cmd.context.client_sequence = request.client_sequence;
        return Result<WorldCommandDto>::ok(std::move(cmd));
    }

    if (request.submit_mode == ClientSubmitMode::WorldCommandDto) {
        if (!request.command.has_value()) {
            return Result<WorldCommandDto>::fail(
                makeError(ErrorCode::command_missing_required_field, "WorldCommandDto mode requires command")
            );
        }
        auto cmd = request.command.value();

        // Enforce session policy on client-submitted commands.
        auto snapshot_result = session_gateway_.getAvailableCommands(request.session_id);
        if (snapshot_result.is_error()) {
            return Result<WorldCommandDto>::fail(snapshot_result.errors());
        }
        auto policy_result = validateClientCommandDto(cmd, actor_key, snapshot_result.value());
        if (policy_result.is_error()) {
            return Result<WorldCommandDto>::fail(policy_result.errors());
        }

        // Normalize actor to session authority.
        cmd.actor_key = actor_key;
        cmd.context.client_sequence = request.client_sequence;
        return Result<WorldCommandDto>::ok(std::move(cmd));
    }

    return Result<WorldCommandDto>::fail(
        makeError(ErrorCode::command_not_allowed_now, "Unsupported submit_mode")
    );
}

ClientCommandResponse ClientCommandGateway::buildBlockedResponse(
    const std::string& session_id,
    uint64_t client_sequence,
    uint64_t projection_version,
    const std::string& reason_key,
    bool requires_full_refresh,
    const std::string& actor_key) {

    ClientCommandResponse response;
    response.session_id = session_id;
    response.accepted_client_sequence = client_sequence;
    response.base_projection_version = projection_version;
    response.new_projection_version = projection_version;
    response.requires_full_refresh = requires_full_refresh;
    response.result.command_id = "blocked_" + std::to_string(client_sequence);
    response.result.command_key = "blocked";
    response.result.actor_key = actor_key;
    response.result.result_kind = WorldCommandResultKind::Blocked;
    response.result.failure_reason_keys.push_back(reason_key);

    logBlockedClientCommand(session_id, client_sequence, projection_version, actor_key, reason_key);

    auto snapshot_result = session_gateway_.getAvailableCommands(session_id);
    if (snapshot_result.is_ok()) {
        response.available_commands = snapshot_result.value();
    }

    return response;
}

Result<ClientCommandResponse> ClientCommandGateway::handleCommand(
    const ClientCommandRequest& request) {
    auto valid = request.validateBasic();
    if (valid.is_error()) {
        return Result<ClientCommandResponse>::fail(valid.errors());
    }

    if (!session_gateway_.hasSession(request.session_id)) {
        return Result<ClientCommandResponse>::ok(
            buildBlockedResponse(request.session_id, request.client_sequence, 0, "session_not_found")
        );
    }

    auto version_result = session_gateway_.getProjectionVersion(request.session_id);
    if (version_result.is_error()) {
        return Result<ClientCommandResponse>::fail(version_result.errors());
    }
    uint64_t current_version = version_result.value();

    // Get actor_key from session authority. Never trust client selection_context.
    auto actor_result = session_gateway_.getSessionActorKey(request.session_id);
    if (actor_result.is_error()) {
        return Result<ClientCommandResponse>::fail(actor_result.errors());
    }
    std::string actor_key = actor_result.value();

    // Validate client_id matches session.
    auto session_client_result = session_gateway_.getSessionClientId(request.session_id);
    if (session_client_result.is_error()) {
        return Result<ClientCommandResponse>::fail(session_client_result.errors());
    }
    if (request.client_id != session_client_result.value()) {
        auto response = buildBlockedResponse(
            request.session_id, request.client_sequence, current_version, "client_id_mismatch", false, actor_key);
        return Result<ClientCommandResponse>::ok(std::move(response));
    }

    // Version mismatch: block immediately, do not execute.
    if (request.known_projection_version != current_version) {
        auto response = buildBlockedResponse(
            request.session_id, request.client_sequence, current_version,
            "projection_version_stale", true, actor_key);
        response.base_projection_version = request.known_projection_version;
        response.projection_patch = patch_contract_.buildFullRefreshHint(
            request.known_projection_version, current_version);
        return Result<ClientCommandResponse>::ok(std::move(response));
    }

    // Resolve command.
    auto cmd_result = resolveCommand(request, actor_key);
    if (cmd_result.is_error()) {
        auto response = buildBlockedResponse(
            request.session_id, request.client_sequence, current_version, "option_materialize_failed", false, actor_key);
        response.result.failure_reason_keys.clear();
        for (const auto& err : cmd_result.errors()) {
            response.result.failure_reason_keys.push_back(err.message_key);
        }
        return Result<ClientCommandResponse>::ok(std::move(response));
    }

    auto command = cmd_result.value();
    pathfinder::logging::log(
        pathfinder::logging::tag::Command,
        "resolved command session=" + request.session_id +
            " seq=" + std::to_string(request.client_sequence) +
            " actor=" + actor_key +
            " kind=" + pathfinder::world_command::toString(command.command_kind) +
            " key=" + command.command_key +
            " source=" + pathfinder::world_command::toString(command.source) +
            " target={" + targetSummary(command.target) + "}");

    // Helper to emit restored events from ensure results.
    auto emitRestoredEvents = [](std::vector<WorldEventDto>& event_feed,
                                 const pathfinder::foundation::Result<pathfinder::world_generation::WorldRegionEnsureResult>& ensure_res) {
        if (ensure_res.is_error()) return;
        const auto& result = ensure_res.value();
        for (const auto& item : result.item_results) {
            if (item.status == pathfinder::world_generation::WorldRegionEnsureStatus::RestoredSnapshot) {
                WorldEventDto event;
                event.event_id = "lifecycle_restored_" + item.region_key.regionRuntimeId();
                event.event_kind = "Lifecycle:restored";
                event.title_text = "Region restored";
                event.body_text = "Region restored from snapshot";
                event_feed.push_back(std::move(event));
            }
        }
    };

    // Ensure the actor's visibility window around the move target before the
    // command runs. Otherwise newly generated edge regions can miss the current
    // projection patch and appear as temporary fog until the next action.
    std::optional<pathfinder::foundation::Result<pathfinder::world_generation::WorldRegionEnsureResult>> ensure_target_res;
    if (ensure_adapter_ && command.command_kind == WorldCommandKind::Move && command.target.target_coord.has_value()) {
        auto layer_result = session_gateway_.getSessionLayerKey(request.session_id);
        std::string layer_key = layer_result.is_ok() ? layer_result.value() : command.target.target_coord->layer_key;
        world_runtime::WorldCellCoord target_coord{
            command.target.target_coord->x,
            command.target.target_coord->y,
            command.target.target_coord->layer_key.empty() ? layer_key : command.target.target_coord->layer_key
        };
        ensure_target_res = ensure_adapter_->ensureForTargetVision(actor_key, layer_key, target_coord);
        if (ensure_target_res->is_error()) {
            auto response = buildBlockedResponse(
                request.session_id, request.client_sequence, current_version, "region_ensure_failed", false, actor_key);
            return Result<ClientCommandResponse>::ok(std::move(response));
        }
    }

    // Sync pipeline projection version to session's current version before execution.
    // This prevents version crosstalk when multiple sessions share the same pipeline.
    pipeline_.setCurrentProjectionVersion(current_version);

    // Execute via WorldCommandPipeline.
    auto pipeline_result = pipeline_.execute(request.session_id, command);
    if (pipeline_result.is_error()) {
        return Result<ClientCommandResponse>::fail(pipeline_result.errors());
    }

    const auto& pipeline_response = pipeline_result.value();

    // Update session projection version if pipeline advanced it.
    if (pipeline_response.projection_patch.new_projection_version > current_version) {
        session_gateway_.updateProjectionVersion(
            request.session_id, pipeline_response.projection_patch.new_projection_version);
    }

    // Build ClientCommandResponse from pipeline response.
    ClientCommandResponse response;
    response.session_id = pipeline_response.session_id;
    response.accepted_client_sequence = request.client_sequence;
    response.base_projection_version = pipeline_response.projection_patch.base_projection_version;
    response.new_projection_version = pipeline_response.projection_patch.new_projection_version;
    response.result = pipeline_response.result;
    response.projection_patch = pipeline_response.projection_patch;
    response.event_feed = pipeline_response.event_feed;
    response.experiences = pipeline_response.experiences;
    response.frontend_hints = pipeline_response.frontend_hints;
    response.warning_keys = pipeline_response.warning_keys;

    if (post_command_hook_) {
        post_command_hook_(command, response);
    }

    // Client-facing available_commands must be rebuilt after post_command_hook_.
    // The hook can add knowledge learned from the command result; using the
    // pipeline's pre-hook options would hide newly teachable / NPC-usable actions
    // until a manual refresh.
    std::optional<pathfinder::foundation::Result<pathfinder::world_generation::WorldRegionEnsureResult>> ensure_avail_res;
    {
        // P58: Ensure neighboring regions exist before building move options.
        auto layer_result = session_gateway_.getSessionLayerKey(request.session_id);
        std::string layer_key = layer_result.is_ok() ? layer_result.value() : "surface";
        if (ensure_adapter_) {
            ensure_avail_res = ensure_adapter_->ensureForAvailableCommands(actor_key, layer_key);
            if (ensure_avail_res->is_error()) {
                response.warning_keys.push_back("region_ensure_failed");
            } else if (!ensure_avail_res->value().ok) {
                for (const auto& w : ensure_avail_res->value().warning_keys) {
                    response.warning_keys.push_back(w);
                }
                for (const auto& f : ensure_avail_res->value().failure_reason_keys) {
                    response.warning_keys.push_back(f);
                }
            }
        }
        auto options_result = available_command_adapter_.buildOptions(actor_key, layer_key);
        if (options_result.is_ok()) {
            response.available_commands = options_result.value();
        } else {
            response.available_commands = pipeline_response.available_commands;
        }
    }

    // P60: Emit restored events from ensure adapters.
    if (ensure_target_res) {
        emitRestoredEvents(response.event_feed, *ensure_target_res);
    }
    if (ensure_avail_res) {
        emitRestoredEvents(response.event_feed, *ensure_avail_res);
    }

    // If pipeline flagged full refresh, require it.
    if (patch_contract_.requiresFullRefresh(
            response.projection_patch, request.known_projection_version)) {
        response.requires_full_refresh = true;
        response.projection_patch.requires_full_refresh = true;
    }

    // Persist the latest available commands snapshot back to the session.
    // This ensures the next command's option authority uses the most recent state.
    session_gateway_.updateAvailableCommands(
        request.session_id, response.available_commands);

    // P60: Trigger activity window + lifecycle after successful command.
    if (activity_window_service_ && lifecycle_trigger_service_) {
        auto layer_result = session_gateway_.getSessionLayerKey(request.session_id);
        std::string layer_key = layer_result.is_ok() ? layer_result.value() : "surface";
        uint64_t tick = response.new_projection_version;

        // Mark the actor's current region as recently touched after successful world interaction.
        if (response.result.result_kind == WorldCommandResultKind::Succeeded) {
            auto touch_res = activity_window_service_->touchActorRegion(actor_key, layer_key, tick);
            if (touch_res.is_error()) {
                response.warning_keys.push_back("activity_touch_failed");
            }
        }

        world_region_state::WorldRegionSnapshotBuildContext ctx;
        ctx.world_id = world_id_;
        ctx.world_seed = world_seed_;
        ctx.world_tick = tick;
        ctx.state_version = tick;
        ctx.worldgen_profile_key = "sandbox_blank";
        ctx.generator_version = "1.0.0";
        ctx.content_version = "1.0.0";

        auto window_res = activity_window_service_->computeWindow(actor_key, layer_key, tick);
        if (window_res.is_ok()) {
            auto trigger_result = lifecycle_trigger_service_->processWindow(window_res.value(), ctx);
            auto shouldExposeLifecycleEvent = [](const std::string& event_kind) {
                return event_kind == "seal_failed";
            };
            for (const auto& ev : trigger_result.events) {
                if (!shouldExposeLifecycleEvent(ev.event_kind)) {
                    continue;
                }
                WorldEventDto event;
                event.event_id = ev.event_id;
                event.event_kind = "Lifecycle:" + ev.event_kind;
                event.tick = ev.tick;
                event.title_text = ev.label_zh;
                event.body_text = ev.label_zh;
                for (const auto& r : ev.failure_reason_keys) {
                    event.reason_keys.push_back(r);
                }
                response.event_feed.push_back(std::move(event));
            }
            for (const auto& w : trigger_result.warning_keys) {
                response.warning_keys.push_back(w);
            }
        }
    }

    // P60: Align protocol projection version with the authoritative runtime after
    // command, available-command ensure, and lifecycle work. These services may
    // generate, restore, seal, or detach regions after the command patch was built.
    {
        auto layer_result = session_gateway_.getSessionLayerKey(request.session_id);
        std::string layer_key = layer_result.is_ok() ? layer_result.value() : "surface";
        auto runtime_version_res = available_command_adapter_.runtimeStateVersion(actor_key, layer_key);
        if (runtime_version_res.is_ok() && runtime_version_res.value() > response.new_projection_version) {
            const uint64_t final_version = runtime_version_res.value();
            response.new_projection_version = final_version;
            response.projection_patch.new_projection_version = final_version;
            session_gateway_.updateProjectionVersion(request.session_id, final_version);
        }
    }

    pathfinder::logging::log(
        pathfinder::logging::tag::Command,
        "completed command session=" + request.session_id +
            " seq=" + std::to_string(request.client_sequence) +
            " actor=" + actor_key +
            " key=" + response.result.command_key +
            " result=" + pathfinder::world_command::toString(response.result.result_kind) +
            " reasons=" + joinReasons(response.result.failure_reason_keys) +
            " base_version=" + std::to_string(response.base_projection_version) +
            " new_version=" + std::to_string(response.new_projection_version) +
            " events=" + std::to_string(response.event_feed.size()) +
            " experiences=" + std::to_string(response.experiences.size()) +
            " available=" + std::to_string(response.available_commands.size()) +
            " full_refresh=" + (response.requires_full_refresh ? std::string("true") : std::string("false")));

    return Result<ClientCommandResponse>::ok(std::move(response));
}

} // namespace pathfinder::client_protocol
