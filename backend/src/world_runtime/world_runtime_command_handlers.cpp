#include "pathfinder/world_runtime/world_command_runtime_handlers.h"
#include "pathfinder/world_runtime/world_command_runtime_bridge.h"
#include "pathfinder/world_generation/move_target_region_guard.h"
#include "pathfinder/world_generation/world_region_ensure_types.h"
#include <memory>

namespace pathfinder::world_runtime {

using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandContext;
using pathfinder::world_command::WorldCommandExecutionResult;
using pathfinder::world_command::WorldCommandResultKind;
using pathfinder::world_command::IWorldCommandHandler;
using pathfinder::world_generation::MoveTargetRegionGuard;

// ---------------------------------------------------------------------------
// GenerateWorld
// ---------------------------------------------------------------------------
class GenerateWorldCommandHandlerRuntime : public IWorldCommandHandler {
public:
    explicit GenerateWorldCommandHandlerRuntime(IWorldRuntime& runtime)
        : bridge_(runtime) {}

    WorldCommandKind kind() const override {
        return WorldCommandKind::GenerateWorld;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {
        return bridge_.handleGenerateWorld(context, command);
    }

private:
    mutable WorldCommandRuntimeBridge bridge_;
};

std::shared_ptr<IWorldCommandHandler> createGenerateWorldCommandHandler(IWorldRuntime& runtime) {
    return std::make_shared<GenerateWorldCommandHandlerRuntime>(runtime);
}

// ---------------------------------------------------------------------------
// Move
// ---------------------------------------------------------------------------
class MoveCommandHandler : public IWorldCommandHandler {
public:
    explicit MoveCommandHandler(IWorldRuntime& runtime, MoveTargetRegionGuard* guard)
        : bridge_(runtime)
        , guard_(guard) {}

    WorldCommandKind kind() const override {
        return WorldCommandKind::Move;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {

        // P58: Ensure target region exists before attempting move.
        if (guard_ && command.target.target_coord) {
            WorldCellCoord target{
                command.target.target_coord->x,
                command.target.target_coord->y,
                command.target.target_coord->layer_key
            };
            auto ensure_res = guard_->ensureMoveTarget(
                command.actor_key,
                target,
                bridge_.worldId(),
                bridge_.worldSeed(),
                "1.0.0",
                "1.0.0",
                "first_world",
                target.layer_key,
                bridge_.regionSize());
            if (ensure_res.is_error()) {
                WorldCommandExecutionResult blocked_result;
                blocked_result.result_kind = WorldCommandResultKind::Blocked;
                blocked_result.failure_reason_keys.push_back("region_ensure_failed");
                return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(blocked_result));
            }
            const auto& ensure_result = ensure_res.value();
            if (!ensure_result.ok) {
                bool any_failed = false;
                for (const auto& item : ensure_result.item_results) {
                    if (item.status == pathfinder::world_generation::WorldRegionEnsureStatus::GenerationFailed ||
                        item.status == pathfinder::world_generation::WorldRegionEnsureStatus::ApplyFailed ||
                        item.status == pathfinder::world_generation::WorldRegionEnsureStatus::InvalidRequest ||
                        item.status == pathfinder::world_generation::WorldRegionEnsureStatus::LimitExceeded) {
                        any_failed = true;
                        break;
                    }
                }
                if (any_failed) {
                    WorldCommandExecutionResult blocked_result;
                    blocked_result.result_kind = WorldCommandResultKind::Blocked;
                    blocked_result.failure_reason_keys.push_back("region_ensure_failed");
                    return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(blocked_result));
                }
            }
        }

        return bridge_.handleMove(context, command);
    }

private:
    mutable WorldCommandRuntimeBridge bridge_;
    MoveTargetRegionGuard* guard_;
};

std::shared_ptr<IWorldCommandHandler> createMoveCommandHandler(IWorldRuntime& runtime, MoveTargetRegionGuard* guard) {
    return std::make_shared<MoveCommandHandler>(runtime, guard);
}

// ---------------------------------------------------------------------------
// Inspect
// ---------------------------------------------------------------------------
class InspectCommandHandlerRuntime : public IWorldCommandHandler {
public:
    explicit InspectCommandHandlerRuntime(IWorldRuntime& runtime)
        : bridge_(runtime) {}

    WorldCommandKind kind() const override {
        return WorldCommandKind::Inspect;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {
        return bridge_.handleInspect(context, command);
    }

private:
    mutable WorldCommandRuntimeBridge bridge_;
};

std::shared_ptr<IWorldCommandHandler> createInspectCommandHandler(IWorldRuntime& runtime) {
    return std::make_shared<InspectCommandHandlerRuntime>(runtime);
}

// ---------------------------------------------------------------------------
// Wait
// ---------------------------------------------------------------------------
class WaitCommandHandlerRuntime : public IWorldCommandHandler {
public:
    explicit WaitCommandHandlerRuntime(IWorldRuntime& runtime)
        : bridge_(runtime) {}

    WorldCommandKind kind() const override {
        return WorldCommandKind::Wait;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {
        return bridge_.handleWait(context, command);
    }

private:
    mutable WorldCommandRuntimeBridge bridge_;
};

std::shared_ptr<IWorldCommandHandler> createWaitCommandHandler(IWorldRuntime& runtime) {
    return std::make_shared<WaitCommandHandlerRuntime>(runtime);
}


// ---------------------------------------------------------------------------
// Attack
// ---------------------------------------------------------------------------
class AttackCommandHandlerRuntime : public IWorldCommandHandler {
public:
    explicit AttackCommandHandlerRuntime(IWorldRuntime& runtime)
        : bridge_(runtime) {}

    WorldCommandKind kind() const override {
        return WorldCommandKind::Attack;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {
        return bridge_.handleAttack(context, command);
    }

private:
    mutable WorldCommandRuntimeBridge bridge_;
};

std::shared_ptr<IWorldCommandHandler> createAttackCommandHandler(IWorldRuntime& runtime) {
    return std::make_shared<AttackCommandHandlerRuntime>(runtime);
}

} // namespace pathfinder::world_runtime
