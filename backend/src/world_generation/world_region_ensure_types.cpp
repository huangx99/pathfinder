#include "pathfinder/world_generation/world_region_ensure_types.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_generation {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

std::string toString(WorldRegionEnsureKind kind) {
    switch (kind) {
        case WorldRegionEnsureKind::BootstrapWindow: return "BootstrapWindow";
        case WorldRegionEnsureKind::RefreshProjectionWindow: return "RefreshProjectionWindow";
        case WorldRegionEnsureKind::AvailableCommandWindow: return "AvailableCommandWindow";
        case WorldRegionEnsureKind::MoveTarget: return "MoveTarget";
        case WorldRegionEnsureKind::CommandExecution: return "CommandExecution";
        case WorldRegionEnsureKind::AgentPlanning: return "AgentPlanning";
        case WorldRegionEnsureKind::SystemPrewarm: return "SystemPrewarm";
        case WorldRegionEnsureKind::TestOnly: return "TestOnly";
        default: return "Unknown";
    }
}

std::string toString(WorldRegionEnsureStatus status) {
    switch (status) {
        case WorldRegionEnsureStatus::AlreadyAvailable: return "AlreadyAvailable";
        case WorldRegionEnsureStatus::GeneratedAndApplied: return "GeneratedAndApplied";
        case WorldRegionEnsureStatus::SkippedByPolicy: return "SkippedByPolicy";
        case WorldRegionEnsureStatus::GenerationFailed: return "GenerationFailed";
        case WorldRegionEnsureStatus::ApplyFailed: return "ApplyFailed";
        case WorldRegionEnsureStatus::InvalidRequest: return "InvalidRequest";
        case WorldRegionEnsureStatus::LimitExceeded: return "LimitExceeded";
        case WorldRegionEnsureStatus::TestOnly: return "TestOnly";
        default: return "Unknown";
    }
}

std::string toString(WorldRegionCoverageMode mode) {
    switch (mode) {
        case WorldRegionCoverageMode::ExactCoords: return "ExactCoords";
        case WorldRegionCoverageMode::ActorVisionWindow: return "ActorVisionWindow";
        case WorldRegionCoverageMode::ActorNeighborMoves: return "ActorNeighborMoves";
        case WorldRegionCoverageMode::RectWindow: return "RectWindow";
        case WorldRegionCoverageMode::RegionList: return "RegionList";
        case WorldRegionCoverageMode::TestOnly: return "TestOnly";
        default: return "Unknown";
    }
}

std::string WorldRegionKey::toString() const {
    return world_id + ":" + layer_key + ":region:" + std::to_string(rx) + ":" + std::to_string(ry) + ":" + std::to_string(region_size);
}

bool WorldRegionKey::operator==(const WorldRegionKey& other) const {
    return world_id == other.world_id && layer_key == other.layer_key
        && rx == other.rx && ry == other.ry && region_size == other.region_size;
}

bool WorldRegionKey::operator<(const WorldRegionKey& other) const {
    if (world_id != other.world_id) return world_id < other.world_id;
    if (layer_key != other.layer_key) return layer_key < other.layer_key;
    if (rx != other.rx) return rx < other.rx;
    if (ry != other.ry) return ry < other.ry;
    return region_size < other.region_size;
}

Result<void> WorldRegionEnsureRequest::validateBasic() const {
    if (world_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "world_id_empty"));
    }
    if (generator_version.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "generator_version_empty"));
    }
    if (content_version.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "content_version_empty"));
    }
    if (worldgen_profile_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "profile_key_empty"));
    }
    if (layer_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "layer_key_empty"));
    }
    if (region_size <= 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "region_size_invalid"));
    }
    if (ensure_kind == WorldRegionEnsureKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ensure_kind_unknown"));
    }
    if (coverage_mode == WorldRegionCoverageMode::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "coverage_mode_unknown"));
    }
    if (max_regions_to_generate < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "max_regions_negative"));
    }
    return Result<void>::ok();
}

} // namespace pathfinder::world_generation
