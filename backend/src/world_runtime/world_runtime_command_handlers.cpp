#include "pathfinder/world_runtime/world_command_runtime_handlers.h"
#include "pathfinder/world_runtime/world_command_runtime_bridge.h"
#include <memory>

namespace pathfinder::world_runtime {

using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandContext;
using pathfinder::world_command::WorldCommandExecutionResult;
using pathfinder::world_command::IWorldCommandHandler;

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
    explicit MoveCommandHandler(IWorldRuntime& runtime)
        : bridge_(runtime) {}

    WorldCommandKind kind() const override {
        return WorldCommandKind::Move;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {
        return bridge_.handleMove(context, command);
    }

private:
    mutable WorldCommandRuntimeBridge bridge_;
};

std::shared_ptr<IWorldCommandHandler> createMoveCommandHandler(IWorldRuntime& runtime) {
    return std::make_shared<MoveCommandHandler>(runtime);
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

} // namespace pathfinder::world_runtime
