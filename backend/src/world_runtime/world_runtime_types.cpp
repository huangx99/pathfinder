#include "pathfinder/world_runtime/world_runtime_types.h"

namespace pathfinder::world_runtime {

// WorldLayerPolicy
std::string toString(WorldLayerPolicy policy) {
    switch (policy) {
        case WorldLayerPolicy::SurfaceOnly: return "SurfaceOnly";
        case WorldLayerPolicy::ExplicitLayerOnly: return "ExplicitLayerOnly";
        case WorldLayerPolicy::AllowLayerSpawn: return "AllowLayerSpawn";
    }
    return "Unknown";
}

// WorldCellVisibility
std::string toString(WorldCellVisibility visibility) {
    switch (visibility) {
        case WorldCellVisibility::Unknown: return "Unknown";
        case WorldCellVisibility::Discovered: return "Discovered";
        case WorldCellVisibility::Visible: return "Visible";
    }
    return "Unknown";
}

// WorldEntityLocationKind
std::string toString(WorldEntityLocationKind kind) {
    switch (kind) {
        case WorldEntityLocationKind::Nowhere: return "Nowhere";
        case WorldEntityLocationKind::OnMap: return "OnMap";
        case WorldEntityLocationKind::InInventory: return "InInventory";
        case WorldEntityLocationKind::InContainer: return "InContainer";
        case WorldEntityLocationKind::Equipped: return "Equipped";
    }
    return "Unknown";
}

// WorldMoveBlockReason
std::string toString(WorldMoveBlockReason reason) {
    switch (reason) {
        case WorldMoveBlockReason::None: return "None";
        case WorldMoveBlockReason::OutOfBounds: return "OutOfBounds";
        case WorldMoveBlockReason::UnknownLayer: return "UnknownLayer";
        case WorldMoveBlockReason::NotAdjacent: return "NotAdjacent";
        case WorldMoveBlockReason::CellBlocked: return "CellBlocked";
        case WorldMoveBlockReason::EntityBlocked: return "EntityBlocked";
        case WorldMoveBlockReason::ActorMissing: return "ActorMissing";
        case WorldMoveBlockReason::TargetMissing: return "TargetMissing";
    }
    return "Unknown";
}

// WorldCellCoord
std::string WorldCellCoord::cellId() const {
    return layer_key + ":" + std::to_string(x) + ":" + std::to_string(y);
}

bool WorldCellCoord::operator==(const WorldCellCoord& other) const {
    return x == other.x && y == other.y && layer_key == other.layer_key;
}

bool WorldCellCoord::operator<(const WorldCellCoord& other) const {
    if (layer_key != other.layer_key) return layer_key < other.layer_key;
    if (x != other.x) return x < other.x;
    return y < other.y;
}

} // namespace pathfinder::world_runtime
