#include "pathfinder/world_agent_execution/agent_plan_command_compiler.h"
#include "pathfinder/foundation/error.h"
#include <cmath>

namespace pathfinder::world_agent_execution {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandSource;
using pathfinder::world_command::WorldCommandTargetDto;
using pathfinder::world_command::WorldCommandTargetKind;
using pathfinder::world_runtime::WorldCellCoord;

AgentPlanCommandCompiler::AgentPlanCommandCompiler() {
}

WorldAgentStepCommandKind AgentPlanCommandCompiler::mapActionToStepKind(const std::string& action_key) const {
    if (action_key == "move") return WorldAgentStepCommandKind::Move;
    if (action_key == "gather") return WorldAgentStepCommandKind::Gather;
    if (action_key == "chop") return WorldAgentStepCommandKind::Chop;
    if (action_key == "mine") return WorldAgentStepCommandKind::Mine;
    if (action_key == "dig") return WorldAgentStepCommandKind::Dig;
    if (action_key == "pickup") return WorldAgentStepCommandKind::Pickup;
    if (action_key == "drop") return WorldAgentStepCommandKind::Drop;
    if (action_key == "eat") return WorldAgentStepCommandKind::Eat;
    if (action_key == "use") return WorldAgentStepCommandKind::Use;
    if (action_key == "craft") return WorldAgentStepCommandKind::Craft;
    if (action_key == "teach") return WorldAgentStepCommandKind::Teach;
    if (action_key == "attack") return WorldAgentStepCommandKind::Attack;
    if (action_key == "flee") return WorldAgentStepCommandKind::Flee;
    if (action_key == "wait") return WorldAgentStepCommandKind::Wait;
    if (action_key == "cast_area_effect") return WorldAgentStepCommandKind::CastAreaEffect;
    if (action_key == "trigger_area_event") return WorldAgentStepCommandKind::TriggerAreaEvent;
    return WorldAgentStepCommandKind::Unknown;
}

WorldCommandKind AgentPlanCommandCompiler::mapStepToCommandKind(WorldAgentStepCommandKind step_kind) const {
    switch (step_kind) {
        case WorldAgentStepCommandKind::Move: return WorldCommandKind::Move;
        case WorldAgentStepCommandKind::Gather: return WorldCommandKind::Gather;
        case WorldAgentStepCommandKind::Chop: return WorldCommandKind::Chop;
        case WorldAgentStepCommandKind::Mine: return WorldCommandKind::Mine;
        case WorldAgentStepCommandKind::Dig: return WorldCommandKind::Dig;
        case WorldAgentStepCommandKind::Pickup: return WorldCommandKind::Pickup;
        case WorldAgentStepCommandKind::Drop: return WorldCommandKind::Drop;
        case WorldAgentStepCommandKind::Eat: return WorldCommandKind::Eat;
        case WorldAgentStepCommandKind::Use: return WorldCommandKind::Use;
        case WorldAgentStepCommandKind::Craft: return WorldCommandKind::Craft;
        case WorldAgentStepCommandKind::Teach: return WorldCommandKind::Teach;
        case WorldAgentStepCommandKind::Attack: return WorldCommandKind::Attack;
        case WorldAgentStepCommandKind::Flee: return WorldCommandKind::Flee;
        case WorldAgentStepCommandKind::Wait: return WorldCommandKind::Wait;
        case WorldAgentStepCommandKind::CastAreaEffect: return WorldCommandKind::CastAreaEffect;
        case WorldAgentStepCommandKind::TriggerAreaEvent: return WorldCommandKind::TriggerAreaEvent;
        default: return WorldCommandKind::Unknown;
    }
}

bool AgentPlanCommandCompiler::isAdjacent(const WorldCellCoord& from, const WorldCellCoord& to) const {
    int dx = std::abs(from.x - to.x);
    int dy = std::abs(from.y - to.y);
    return dx <= 1 && dy <= 1 && !(dx == 0 && dy == 0);
}

Result<WorldCommandDto> AgentPlanCommandCompiler::buildMoveCommand(
    const std::string& actor_key,
    const pathfinder::agent_reasoning::AgentPlanStep& step,
    const WorldAgentDecisionContext& context,
    const std::string& command_id) const {
    WorldCommandDto cmd;
    cmd.command_id = command_id;
    cmd.command_kind = WorldCommandKind::Move;
    cmd.command_key = pathfinder::world_command::toString(cmd.command_kind);
    cmd.source = WorldCommandSource::AgentDecision;
    cmd.actor_key = actor_key;
    cmd.context.parent_command_id = context.context_id;
    cmd.context.issued_tick = context.tick;

    // Determine target coordinate from step or from visible resources
    if (step.target_key.has_value() && !step.target_key->empty()) {
        // Look up target in visible resources or entities
        for (const auto& res : context.visible_resources) {
            if (res.node_id == step.target_key.value() || res.resource_key == step.target_key.value()) {
                cmd.target.target_kind = WorldCommandTargetKind::Coordinate;
                cmd.target.target_coord = pathfinder::world_command::WorldCoordinateDto{res.coord.x, res.coord.y, res.coord.layer_key};
                break;
            }
        }
        if (!cmd.target.target_coord.has_value()) {
            for (const auto& ent : context.visible_entities) {
                if (ent.entity_id == step.target_key.value() || ent.entity_key == step.target_key.value()) {
                    cmd.target.target_kind = WorldCommandTargetKind::Coordinate;
                    cmd.target.target_coord = pathfinder::world_command::WorldCoordinateDto{ent.coord.x, ent.coord.y, ent.coord.layer_key};
                    break;
                }
            }
        }
    }

    if (!cmd.target.target_coord.has_value()) {
        return Result<WorldCommandDto>::fail(
            makeError(ErrorCode::command_invalid_argument, "move_target_coord_unresolved")
        );
    }

    return Result<WorldCommandDto>::ok(cmd);
}

Result<WorldCommandDto> AgentPlanCommandCompiler::buildGatherCommand(
    const std::string& actor_key,
    const pathfinder::agent_reasoning::AgentPlanStep& step,
    const std::string& command_id) const {
    WorldCommandDto cmd;
    cmd.command_id = command_id;
    cmd.command_kind = WorldCommandKind::Gather;
    cmd.command_key = pathfinder::world_command::toString(cmd.command_kind);
    cmd.source = WorldCommandSource::AgentDecision;
    cmd.actor_key = actor_key;
    if (step.target_key.has_value()) {
        cmd.target.target_kind = WorldCommandTargetKind::Entity;
        cmd.target.target_entity_id = step.target_key.value();
    }
    if (!step.object_key.empty()) {
        cmd.parameters["resource_key"] = step.object_key;
    }
    return Result<WorldCommandDto>::ok(cmd);
}

Result<WorldCommandDto> AgentPlanCommandCompiler::buildGenericCommand(
    const std::string& actor_key,
    const pathfinder::agent_reasoning::AgentPlanStep& step,
    WorldAgentStepCommandKind step_kind,
    const std::string& command_id) const {
    WorldCommandDto cmd;
    cmd.command_id = command_id;
    cmd.command_kind = mapStepToCommandKind(step_kind);
    cmd.command_key = pathfinder::world_command::toString(cmd.command_kind);
    cmd.source = WorldCommandSource::AgentDecision;
    cmd.actor_key = actor_key;
    if (step.target_key.has_value()) {
        cmd.target.target_kind = WorldCommandTargetKind::Entity;
        cmd.target.target_entity_id = step.target_key.value();
    }
    if (!step.object_key.empty()) {
        cmd.parameters["object_key"] = step.object_key;
    }
    if (!step.effect_key.empty()) {
        cmd.parameters["effect_key"] = step.effect_key;
    }
    return Result<WorldCommandDto>::ok(cmd);
}

Result<WorldAgentCompiledCommand> AgentPlanCommandCompiler::compile(
    const std::string& actor_key,
    const pathfinder::agent_reasoning::AgentPlanStep& step,
    const WorldAgentDecisionContext& context,
    const std::string& tick_request_id) {

    WorldAgentCompiledCommand compiled;
    auto step_kind = mapActionToStepKind(step.action_key);
    if (step_kind == WorldAgentStepCommandKind::Unknown) {
        compiled.ok = false;
        pathfinder::goal_execution::InternalBlocker blocker;
        blocker.kind = pathfinder::goal_execution::InternalBlockerKind::KnowledgeMissing;
        blocker.actor_key = actor_key;
        blocker.safe_summary_zh_cn = "unknown_action_kind";
        compiled.blockers.push_back(blocker);
        return Result<WorldAgentCompiledCommand>::ok(compiled);
    }

    std::string command_id = tick_request_id + "." + step.step_id + ".0";

    // Distance check for gather/chop/mine/pickup/use: must be adjacent
    if (step_kind == WorldAgentStepCommandKind::Gather ||
        step_kind == WorldAgentStepCommandKind::Chop ||
        step_kind == WorldAgentStepCommandKind::Mine ||
        step_kind == WorldAgentStepCommandKind::Pickup ||
        step_kind == WorldAgentStepCommandKind::Use) {

        bool found_target = false;
        WorldCellCoord target_coord;
        if (step.target_key.has_value() && !step.target_key->empty()) {
            for (const auto& res : context.visible_resources) {
                if (res.node_id == step.target_key.value()) {
                    found_target = true;
                    target_coord = res.coord;
                    break;
                }
            }
            if (!found_target) {
                for (const auto& ent : context.visible_entities) {
                    if (ent.entity_id == step.target_key.value()) {
                        found_target = true;
                        target_coord = ent.coord;
                        break;
                    }
                }
            }
        }

        // Fallback: if target_key is a resource_key rather than node_id, look up by object_key
        if (!found_target && !step.object_key.empty()) {
            for (const auto& res : context.visible_resources) {
                if (res.resource_key == step.object_key) {
                    found_target = true;
                    target_coord = res.coord;
                    break;
                }
            }
        }

        if (found_target && !isAdjacent(context.actor.coord, target_coord)) {
            // Not adjacent: emit Move instead
            auto move_cmd = buildMoveCommand(actor_key, step, context, command_id);
            if (move_cmd.is_ok()) {
                compiled.ok = true;
                compiled.step_command_kind = WorldAgentStepCommandKind::Move;
                compiled.command = move_cmd.value();
            } else {
                compiled.ok = false;
            }
            return Result<WorldAgentCompiledCommand>::ok(compiled);
        }
    }

    std::optional<Result<WorldCommandDto>> cmd_result_opt;
    if (step_kind == WorldAgentStepCommandKind::Move) {
        cmd_result_opt = buildMoveCommand(actor_key, step, context, command_id);
    } else if (step_kind == WorldAgentStepCommandKind::Gather) {
        cmd_result_opt = buildGatherCommand(actor_key, step, command_id);
    } else {
        cmd_result_opt = buildGenericCommand(actor_key, step, step_kind, command_id);
    }

    if (!cmd_result_opt.has_value() || cmd_result_opt.value().is_error()) {
        compiled.ok = false;
        return Result<WorldAgentCompiledCommand>::ok(compiled);
    }

    compiled.ok = true;
    compiled.step_command_kind = step_kind;
    compiled.command = cmd_result_opt.value().value();
    compiled.command.context.parent_command_id = tick_request_id;
    compiled.command.context.issued_tick = context.tick;

    auto compiled_validation = compiled.validateBasic();
    if (compiled_validation.is_error()) {
        compiled.ok = false;
        pathfinder::goal_execution::InternalBlocker blocker;
        blocker.kind = pathfinder::goal_execution::InternalBlockerKind::MissingCondition;
        blocker.actor_key = actor_key;
        blocker.safe_summary_zh_cn = "compiled_command_invalid";
        compiled.blockers.push_back(blocker);
        compiled.reason_keys.push_back("compiled_command_validation_failed");
    }

    return Result<WorldAgentCompiledCommand>::ok(compiled);
}

} // namespace pathfinder::world_agent_execution
