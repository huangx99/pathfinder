#include "pathfinder/world_command/world_command_pipeline.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_command {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

WorldCommandPipeline::WorldCommandPipeline(WorldCommandDispatcher& dispatcher)
    : dispatcher_(dispatcher) {}

uint64_t WorldCommandPipeline::currentProjectionVersion() const {
    return projection_version_;
}

void WorldCommandPipeline::setCurrentProjectionVersion(uint64_t version) {
    projection_version_ = version;
}

Result<WorldCommandResponseDto> WorldCommandPipeline::execute(
    const std::string& session_id,
    const WorldCommandDto& command) {

    trace_recorder_.clear();

    WorldCommandResponseDto response;
    response.session_id = session_id;
    response.command_id = command.command_id;

    trace_recorder_.recordCommandReceived(command);

    // 1. ReceiveCommand (implicit)
    trace_recorder_.recordStage("ReceiveCommand");

    // 2. NormalizeCommand
    trace_recorder_.recordStage("NormalizeCommand");
    WorldCommandDto normalized = normalizeCommand(command);
    trace_recorder_.recordNormalized(normalized);

    // 3. ValidateCommandShape
    trace_recorder_.recordStage("ValidateCommandShape");
    auto shape_result = validateCommandShape(normalized);
    if (shape_result.is_error()) {
        trace_recorder_.recordValidated(false, {shape_result.errors().front().message_key});
        response.result.command_id = normalized.command_id;
        response.result.command_key = normalized.command_key;
        response.result.result_kind = WorldCommandResultKind::Failed;
        response.result.failure_reason_keys.push_back("invalid_command_shape");
        trace_recorder_.recordResult(response.result);
        response.projection_patch = patch_builder_.setVersions(projection_version_, projection_version_).build().value();
        response.available_commands = available_builder_.build(WorldCommandContext(normalized)).value();
        response.frontend_hints = hint_builder_.build(WorldCommandContext(normalized), WorldCommandExecutionResult{}).value();
        return Result<WorldCommandResponseDto>::ok(std::move(response));
    }

    // 4-6. ResolveActor, ResolveTarget, ValidateSource
    trace_recorder_.recordStage("ResolveActor");
    auto actor_result = resolveActor(normalized);
    if (actor_result.is_error()) {
        trace_recorder_.recordValidated(false, {actor_result.errors().front().message_key});
        response.result.command_id = normalized.command_id;
        response.result.command_key = normalized.command_key;
        response.result.result_kind = WorldCommandResultKind::Failed;
        response.result.failure_reason_keys.push_back("invalid_actor");
        trace_recorder_.recordResult(response.result);
        response.projection_patch = patch_builder_.setVersions(projection_version_, projection_version_).build().value();
        response.available_commands = available_builder_.build(WorldCommandContext(normalized)).value();
        response.frontend_hints = hint_builder_.build(WorldCommandContext(normalized), WorldCommandExecutionResult{}).value();
        return Result<WorldCommandResponseDto>::ok(std::move(response));
    }

    trace_recorder_.recordStage("ResolveTarget");
    auto target_result = resolveTarget(normalized);
    if (target_result.is_error()) {
        trace_recorder_.recordValidated(false, {target_result.errors().front().message_key});
        response.result.command_id = normalized.command_id;
        response.result.command_key = normalized.command_key;
        response.result.result_kind = WorldCommandResultKind::Failed;
        response.result.failure_reason_keys.push_back("invalid_target");
        trace_recorder_.recordResult(response.result);
        response.projection_patch = patch_builder_.setVersions(projection_version_, projection_version_).build().value();
        response.available_commands = available_builder_.build(WorldCommandContext(normalized)).value();
        response.frontend_hints = hint_builder_.build(WorldCommandContext(normalized), WorldCommandExecutionResult{}).value();
        return Result<WorldCommandResponseDto>::ok(std::move(response));
    }

    trace_recorder_.recordStage("ValidateSource");
    auto source_result = validateSource(normalized);
    if (source_result.is_error()) {
        trace_recorder_.recordValidated(false, {source_result.errors().front().message_key});
        response.result.command_id = normalized.command_id;
        response.result.command_key = normalized.command_key;
        response.result.result_kind = WorldCommandResultKind::Blocked;
        response.result.failure_reason_keys.push_back("invalid_source");
        trace_recorder_.recordResult(response.result);
        response.projection_patch = patch_builder_.setVersions(projection_version_, projection_version_).build().value();
        response.available_commands = available_builder_.build(WorldCommandContext(normalized)).value();
        response.frontend_hints = hint_builder_.build(WorldCommandContext(normalized), WorldCommandExecutionResult{}).value();
        return Result<WorldCommandResponseDto>::ok(std::move(response));
    }

    // 7-11. Stub validations (P43)
    trace_recorder_.recordStage("ValidateLayer");
    validateLayer(normalized);
    trace_recorder_.recordStage("ValidateRange");
    validateRange(normalized);
    trace_recorder_.recordStage("ValidateKnowledgeGate");
    validateKnowledgeGate(normalized);
    trace_recorder_.recordStage("ValidateConditions");
    validateConditions(normalized);
    trace_recorder_.recordStage("ValidateMaterials");
    validateMaterials(normalized);

    trace_recorder_.recordValidated(true, {});

    // 12-14. Dispatch to handler
    trace_recorder_.recordStage("BuildEffectPlan");
    trace_recorder_.recordStage("SimulateStateDelta");
    trace_recorder_.recordStage("ApplyStateDelta");

    WorldCommandContext context(normalized);
    context.setCurrentTick(normalized.context.issued_tick);
    for (const auto& stage : trace_recorder_.getTrace()) {
        context.addTraceStage(stage);
    }

    auto dispatch_result = dispatcher_.dispatch(context, normalized);
    if (dispatch_result.is_error()) {
        response.result.command_id = normalized.command_id;
        response.result.command_key = normalized.command_key;
        response.result.result_kind = WorldCommandResultKind::Failed;
        response.result.failure_reason_keys.push_back("dispatch_failed");
        trace_recorder_.recordResult(response.result);
        response.projection_patch = patch_builder_.setVersions(projection_version_, projection_version_).build().value();
        response.available_commands = available_builder_.build(context).value();
        response.frontend_hints = hint_builder_.build(context, WorldCommandExecutionResult{}).value();
        return Result<WorldCommandResponseDto>::ok(std::move(response));
    }

    WorldCommandExecutionResult execution = dispatch_result.value();

    // 15-17. Emit events, experiences, spawn followups
    trace_recorder_.recordStage("EmitWorldEvents");
    trace_recorder_.recordStage("EmitExperiences");
    trace_recorder_.recordStage("SpawnFollowupCommands");

    // Build result DTO
    response.result.command_id = normalized.command_id;
    response.result.command_key = normalized.command_key;
    response.result.result_kind = execution.result_kind;
    response.result.actor_key = normalized.actor_key;
    response.result.failure_reason_keys = execution.failure_reason_keys;
    response.result.warning_keys = execution.warning_keys;
    response.result.state_deltas = execution.state_deltas;
    response.result.spawned_commands = execution.spawned_commands;

    // 18. BuildProjectionPatch
    trace_recorder_.recordStage("BuildProjectionPatch");
    uint64_t new_version = projection_version_ + 1;

    if (execution.projection_patch_override.has_value()) {
        // Runtime-aware handler provided a full patch; merge versions
        response.projection_patch = execution.projection_patch_override.value();
        response.projection_patch.base_projection_version = projection_version_;
        response.projection_patch.new_projection_version = new_version;
    } else if (!execution.changed_cell_ids.empty() || !execution.changed_entity_ids.empty() || execution.requires_full_refresh) {
        // Handler provided changed refs but no pre-built patch; build default patch with refs
        auto patch_result = patch_builder_.setVersions(projection_version_, new_version)
                                           .setRequiresFullRefresh(execution.requires_full_refresh)
                                           .build();
        if (patch_result.is_ok()) {
            response.projection_patch = patch_result.value();
        }
    } else {
        // Default empty patch
        auto patch_result = patch_builder_.setVersions(projection_version_, new_version).build();
        if (patch_result.is_ok()) {
            response.projection_patch = patch_result.value();
        }
    }
    projection_version_ = new_version;
    trace_recorder_.recordProjectionVersion(projection_version_);

    // 19. BuildAvailableCommands
    trace_recorder_.recordStage("BuildAvailableCommands");
    auto available_result = available_builder_.build(context);
    if (available_result.is_ok()) {
        response.available_commands = available_result.value();
    }

    // Frontend hints
    auto hints_result = hint_builder_.build(context, execution);
    if (hints_result.is_ok()) {
        response.frontend_hints = hints_result.value();
    }

    response.event_feed = execution.events;
    response.experiences = execution.experiences;

    // 20. RecordTrace
    trace_recorder_.recordStage("RecordTrace");
    trace_recorder_.recordResult(response.result);
    response.debug_trace_keys = trace_recorder_.getTrace();

    return Result<WorldCommandResponseDto>::ok(std::move(response));
}

// ---------------------------------------------------------------------------
// Pipeline stage implementations
// ---------------------------------------------------------------------------
WorldCommandDto WorldCommandPipeline::normalizeCommand(const WorldCommandDto& command) {
    WorldCommandDto normalized = command;
    if (normalized.command_key.empty()) {
        normalized.command_key = toString(normalized.command_kind);
    }
    if (normalized.command_kind == WorldCommandKind::Unknown && !normalized.command_key.empty()) {
        auto kind_result = worldCommandKindFromString(normalized.command_key);
        if (kind_result.is_ok()) {
            normalized.command_kind = kind_result.value();
        }
    }
    if (normalized.context.issued_tick == 0) {
        normalized.context.issued_tick = projection_version_;
    }
    return normalized;
}

Result<void> WorldCommandPipeline::validateCommandShape(const WorldCommandDto& command) {
    return command.validateBasic();
}

Result<void> WorldCommandPipeline::resolveActor(const WorldCommandDto& /*command*/) {
    // P43 stub: accept all actors for now
    return Result<void>::ok();
}

Result<void> WorldCommandPipeline::resolveTarget(const WorldCommandDto& /*command*/) {
    // P43 stub: accept all targets for now
    return Result<void>::ok();
}

Result<void> WorldCommandPipeline::validateSource(const WorldCommandDto& /*command*/) {
    // P43 stub: accept all sources for now
    return Result<void>::ok();
}

Result<void> WorldCommandPipeline::validateLayer(const WorldCommandDto& /*command*/) {
    // P43 stub
    return Result<void>::ok();
}

Result<void> WorldCommandPipeline::validateRange(const WorldCommandDto& /*command*/) {
    // P43 stub
    return Result<void>::ok();
}

Result<void> WorldCommandPipeline::validateKnowledgeGate(const WorldCommandDto& /*command*/) {
    // P43 stub
    return Result<void>::ok();
}

Result<void> WorldCommandPipeline::validateConditions(const WorldCommandDto& /*command*/) {
    // P43 stub
    return Result<void>::ok();
}

Result<void> WorldCommandPipeline::validateMaterials(const WorldCommandDto& /*command*/) {
    // P43 stub
    return Result<void>::ok();
}

} // namespace pathfinder::world_command
