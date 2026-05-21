#include "pathfinder/world_beast_ecology/beast_command_compiler.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_beast_ecology {

using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::foundation::ErrorCode;
using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandSource;
using pathfinder::world_command::WorldCommandTargetDto;
using pathfinder::world_command::WorldCommandTargetKind;
using pathfinder::world_command::WorldCommandContextDto;
using pathfinder::world_command::toString;

Result<WorldCommandDto> BeastCommandCompiler::compile(
    const BeastActionIntent& intent,
    const std::string& request_id,
    const std::string& actor_key) {

    WorldCommandDto command;
    command.command_id = request_id + "_" + intent.intent_id;
    command.command_kind = intent.command_kind;
    command.command_key = toString(intent.command_kind);
    command.source = WorldCommandSource::BeastDecision;
    command.actor_key = actor_key;

    WorldCommandTargetDto target;
    switch (intent.kind) {
        case BeastActionIntentKind::Approach:
        case BeastActionIntentKind::Flee:
            if (intent.target_coord.has_value()) {
                target.target_kind = WorldCommandTargetKind::Coordinate;
                target.target_coord = pathfinder::world_command::WorldCoordinateDto{
                    intent.target_coord.value().x,
                    intent.target_coord.value().y,
                    intent.target_coord.value().layer_key
                };
            } else if (!intent.target_ref.empty()) {
                target.target_kind = WorldCommandTargetKind::Actor;
                target.target_actor_key = intent.target_ref;
            }
            break;
        case BeastActionIntentKind::Attack:
        case BeastActionIntentKind::Threaten:
            if (!intent.target_ref.empty()) {
                target.target_kind = WorldCommandTargetKind::Actor;
                target.target_actor_key = intent.target_ref;
            }
            break;
        case BeastActionIntentKind::Wander:
            // P52: without a target_coord, Wander cannot safely compile to Move.
            // Fall back to Wait to avoid hardcoding a destination.
            command.command_kind = pathfinder::world_command::WorldCommandKind::Wait;
            command.command_key = toString(pathfinder::world_command::WorldCommandKind::Wait);
            target.target_kind = WorldCommandTargetKind::None;
            break;
        default:
            target.target_kind = WorldCommandTargetKind::None;
            break;
    }
    command.target = target;

    auto validate = command.validateBasic();
    if (validate.is_error()) {
        return Result<WorldCommandDto>::fail(makeError(ErrorCode::validation_failed, "compiled_command_invalid"));
    }

    return Result<WorldCommandDto>::ok(std::move(command));
}

} // namespace pathfinder::world_beast_ecology
