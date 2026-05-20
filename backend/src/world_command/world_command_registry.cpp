#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_command {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

Result<void> WorldCommandHandlerRegistry::registerHandler(
    std::shared_ptr<IWorldCommandHandler> handler) {
    if (!handler) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "Cannot register null handler")
        );
    }

    auto kind = handler->kind();
    if (kind == WorldCommandKind::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "Cannot register handler with Unknown kind")
        );
    }

    if (kind_registry_.find(kind) != kind_registry_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_duplicate,
                "Handler already registered for kind: " + toString(kind))
        );
    }

    kind_registry_[kind] = handler;
    return Result<void>::ok();
}

Result<void> WorldCommandHandlerRegistry::bindActionKey(
    const std::string& action_key,
    WorldCommandKind handler_kind) {
    if (action_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "action_key cannot be empty")
        );
    }
    if (handler_kind == WorldCommandKind::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::command_unknown_type, "Cannot bind action_key to Unknown kind")
        );
    }
    if (kind_registry_.find(handler_kind) == kind_registry_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_unknown_type,
                "Handler not registered for kind: " + toString(handler_kind))
        );
    }
    if (action_registry_.find(action_key) != action_registry_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_duplicate,
                "action_key already bound: " + action_key)
        );
    }
    action_registry_[action_key] = kind_registry_[handler_kind];
    return Result<void>::ok();
}

const IWorldCommandHandler* WorldCommandHandlerRegistry::findByKind(WorldCommandKind kind) const {
    auto it = kind_registry_.find(kind);
    if (it != kind_registry_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const IWorldCommandHandler* WorldCommandHandlerRegistry::findByActionKey(
    const std::string& action_key) const {
    auto it = action_registry_.find(action_key);
    if (it != action_registry_.end()) {
        return it->second.get();
    }
    // Fallback: try to match by kind string if no explicit action mapping
    auto kind_result = worldCommandKindFromString(action_key);
    if (kind_result.is_ok()) {
        return findByKind(kind_result.value());
    }
    return nullptr;
}

size_t WorldCommandHandlerRegistry::handlerCount() const {
    return kind_registry_.size();
}

void WorldCommandHandlerRegistry::clear() {
    kind_registry_.clear();
    action_registry_.clear();
}

} // namespace pathfinder::world_command
