#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/foundation/error.h"
#include <ctime>

namespace pathfinder::world_generation {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

// ---------------------------------------------------------------------------
// Enum toString implementations
// ---------------------------------------------------------------------------

std::string toString(WorldGenerationFailureKind kind) {
    switch (kind) {
        case WorldGenerationFailureKind::None: return "None";
        case WorldGenerationFailureKind::InvalidRequest: return "InvalidRequest";
        case WorldGenerationFailureKind::ContentMissing: return "ContentMissing";
        case WorldGenerationFailureKind::ProfileNotFound: return "ProfileNotFound";
        case WorldGenerationFailureKind::RegionAlreadyGenerated: return "RegionAlreadyGenerated";
        case WorldGenerationFailureKind::InvalidRegionCoord: return "InvalidRegionCoord";
        case WorldGenerationFailureKind::InvalidLayer: return "InvalidLayer";
        case WorldGenerationFailureKind::DeterminismMismatch: return "DeterminismMismatch";
        case WorldGenerationFailureKind::ApplyFailed: return "ApplyFailed";
        case WorldGenerationFailureKind::RuntimeConflict: return "RuntimeConflict";
    }
    return "Unknown";
}

std::string toString(WorldBiomeKind kind) {
    switch (kind) {
        case WorldBiomeKind::Unknown: return "Unknown";
        case WorldBiomeKind::Plains: return "Plains";
        case WorldBiomeKind::Forest: return "Forest";
        case WorldBiomeKind::Wetland: return "Wetland";
        case WorldBiomeKind::StoneField: return "StoneField";
        case WorldBiomeKind::Cave: return "Cave";
        case WorldBiomeKind::Ruins: return "Ruins";
        case WorldBiomeKind::WaterEdge: return "WaterEdge";
        case WorldBiomeKind::DangerNest: return "DangerNest";
    }
    return "Unknown";
}

std::string toString(ResourceNodeKind kind) {
    switch (kind) {
        case ResourceNodeKind::Unknown: return "Unknown";
        case ResourceNodeKind::Plant: return "Plant";
        case ResourceNodeKind::Tree: return "Tree";
        case ResourceNodeKind::Stone: return "Stone";
        case ResourceNodeKind::Ore: return "Ore";
        case ResourceNodeKind::Soil: return "Soil";
        case ResourceNodeKind::Water: return "Water";
        case ResourceNodeKind::Corpse: return "Corpse";
        case ResourceNodeKind::Nest: return "Nest";
        case ResourceNodeKind::Relic: return "Relic";
    }
    return "Unknown";
}

std::string toString(ResourceNodeState kind) {
    switch (kind) {
        case ResourceNodeState::Unknown: return "Unknown";
        case ResourceNodeState::Available: return "Available";
        case ResourceNodeState::Hidden: return "Hidden";
        case ResourceNodeState::Depleted: return "Depleted";
        case ResourceNodeState::Regenerating: return "Regenerating";
        case ResourceNodeState::Blocked: return "Blocked";
        case ResourceNodeState::Dangerous: return "Dangerous";
    }
    return "Unknown";
}

std::string toString(SpawnPointKind kind) {
    switch (kind) {
        case SpawnPointKind::Unknown: return "Unknown";
        case SpawnPointKind::PlayerStart: return "PlayerStart";
        case SpawnPointKind::CompanionStart: return "CompanionStart";
        case SpawnPointKind::NpcCamp: return "NpcCamp";
        case SpawnPointKind::BeastNest: return "BeastNest";
        case SpawnPointKind::ResourceCluster: return "ResourceCluster";
        case SpawnPointKind::DangerEntrance: return "DangerEntrance";
        case SpawnPointKind::Landmark: return "Landmark";
    }
    return "Unknown";
}

std::string toString(TerrainGenerationMode mode) {
    switch (mode) {
        case TerrainGenerationMode::Unknown: return "Unknown";
        case TerrainGenerationMode::WeightedRandom: return "WeightedRandom";
        case TerrainGenerationMode::NoiseField: return "NoiseField";
        case TerrainGenerationMode::TestOnly: return "TestOnly";
    }
    return "Unknown";
}

std::string toString(NoiseChannelKind kind) {
    switch (kind) {
        case NoiseChannelKind::Unknown: return "Unknown";
        case NoiseChannelKind::Elevation: return "Elevation";
        case NoiseChannelKind::Moisture: return "Moisture";
        case NoiseChannelKind::Temperature: return "Temperature";
        case NoiseChannelKind::Roughness: return "Roughness";
        case NoiseChannelKind::ResourceRichness: return "ResourceRichness";
        case NoiseChannelKind::DangerPressure: return "DangerPressure";
        case NoiseChannelKind::TestOnly: return "TestOnly";
    }
    return "Unknown";
}

std::string toString(NoiseAlgorithmKind kind) {
    switch (kind) {
        case NoiseAlgorithmKind::Unknown: return "Unknown";
        case NoiseAlgorithmKind::ValueNoise2D: return "ValueNoise2D";
        case NoiseAlgorithmKind::Perlin2D: return "Perlin2D";
        case NoiseAlgorithmKind::FractalPerlin2D: return "FractalPerlin2D";
        case NoiseAlgorithmKind::TestOnly: return "TestOnly";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

Result<void> WorldGenerationRequest::validateBasic() const {
    if (world_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "world_id missing"));
    }
    if (region_size <= 0) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "region_size invalid"));
    }
    if (worldgen_profile_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "worldgen_profile_key missing"));
    }
    return Result<void>::ok();
}

Result<void> WorldGenerationResult::validateBasic() const {
    if (!ok && failure_kind == WorldGenerationFailureKind::None) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "failure_kind must be set when ok=false"));
    }
    return Result<void>::ok();
}

} // namespace pathfinder::world_generation
