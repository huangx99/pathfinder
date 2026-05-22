#include "pathfinder/world_generation/world_region_ensure_service.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <set>
#include <sstream>

namespace pathfinder::world_generation {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

WorldRegionEnsureService::WorldRegionEnsureService(
    WorldGenerationService& generation_service,
    world_runtime::IWorldRuntime& world_runtime,
    world_inventory::IWorldEntityLocationPort& location_port)
    : generation_service_(generation_service)
    , world_runtime_(world_runtime)
    , location_port_(location_port) {
}

std::vector<WorldRegionCoord> WorldRegionEnsureService::computeDesiredRegions(
    const WorldRegionEnsureRequest& request) {
    std::set<WorldRegionCoord> regions;

    switch (request.coverage_mode) {
        case WorldRegionCoverageMode::ExactCoords: {
            auto covered = WorldRegionMath::regionsCoveringCoords(request.target_coords, request.region_size);
            regions.insert(covered.begin(), covered.end());
            break;
        }
        case WorldRegionCoverageMode::ActorVisionWindow: {
            if (request.center_coord) {
                int r = request.radius_cells;
                auto covered = WorldRegionMath::regionsCoveringRect(
                    request.center_coord->x - r,
                    request.center_coord->x + r,
                    request.center_coord->y - r,
                    request.center_coord->y + r,
                    request.region_size);
                regions.insert(covered.begin(), covered.end());
            }
            break;
        }
        case WorldRegionCoverageMode::ActorNeighborMoves: {
            if (request.center_coord) {
                std::vector<world_runtime::WorldCellCoord> neighbors;
                neighbors.push_back(world_runtime::WorldCellCoord{request.center_coord->x, request.center_coord->y - 1, request.layer_key});
                neighbors.push_back(world_runtime::WorldCellCoord{request.center_coord->x, request.center_coord->y + 1, request.layer_key});
                neighbors.push_back(world_runtime::WorldCellCoord{request.center_coord->x - 1, request.center_coord->y, request.layer_key});
                neighbors.push_back(world_runtime::WorldCellCoord{request.center_coord->x + 1, request.center_coord->y, request.layer_key});
                auto covered = WorldRegionMath::regionsCoveringCoords(neighbors, request.region_size);
                regions.insert(covered.begin(), covered.end());
            }
            break;
        }
        case WorldRegionCoverageMode::RectWindow: {
            if (request.center_coord) {
                int r = request.radius_cells;
                auto covered = WorldRegionMath::regionsCoveringRect(
                    request.center_coord->x - r,
                    request.center_coord->x + r,
                    request.center_coord->y - r,
                    request.center_coord->y + r,
                    request.region_size);
                regions.insert(covered.begin(), covered.end());
            }
            break;
        }
        case WorldRegionCoverageMode::RegionList: {
            regions.insert(request.explicit_regions.begin(), request.explicit_regions.end());
            break;
        }
        case WorldRegionCoverageMode::TestOnly: {
            regions.insert(request.explicit_regions.begin(), request.explicit_regions.end());
            break;
        }
        default:
            break;
    }

    // Apply preload margin
    if (request.preload_margin_regions > 0 && request.center_coord) {
        auto center_region = WorldRegionMath::coordToRegion(
            request.center_coord->x, request.center_coord->y, request.region_size);
        int margin = request.preload_margin_regions;
        for (int rx = center_region.rx - margin; rx <= center_region.rx + margin; ++rx) {
            for (int ry = center_region.ry - margin; ry <= center_region.ry + margin; ++ry) {
                regions.insert(WorldRegionCoord{rx, ry});
            }
        }
    }

    return std::vector<WorldRegionCoord>(regions.begin(), regions.end());
}

WorldRegionEnsureItemResult WorldRegionEnsureService::ensureSingleRegion(
    const WorldRegionEnsureRequest& request,
    const WorldRegionCoord& region) {

    WorldRegionEnsureItemResult item;
    item.region_key.world_id = request.world_id;
    item.region_key.layer_key = request.layer_key;
    item.region_key.rx = region.rx;
    item.region_key.ry = region.ry;
    item.region_key.region_size = request.region_size;

    std::string region_id = item.region_key.regionRuntimeId();

    // Check if already generated in runtime
    if (world_runtime_.isRegionGenerated(region_id)) {
        item.status = WorldRegionEnsureStatus::AlreadyAvailable;
        return item;
    }

    // Check if already generated in service-level cache
    if (generation_service_.isRegionGenerated(request.world_id, region)) {
        // Service says generated but runtime doesn't know - this is inconsistent.
        // Treat as already available to avoid re-generation, but warn.
        item.status = WorldRegionEnsureStatus::AlreadyAvailable;
        item.warning_keys.push_back("service_runtime_inconsistent");
        return item;
    }

    if (!request.allow_generate) {
        item.status = WorldRegionEnsureStatus::SkippedByPolicy;
        return item;
    }

    // Build generation request
    WorldGenerationRequest gen_request;
    gen_request.world_id = request.world_id;
    gen_request.world_seed = request.world_seed;
    gen_request.generator_version = request.generator_version;
    gen_request.content_version = request.content_version;
    gen_request.worldgen_profile_key = request.worldgen_profile_key;
    gen_request.region_coord = region;
    gen_request.region_size = request.region_size;
    gen_request.enabled_layer_keys = std::vector<std::string>{request.layer_key};

    auto gen_result = generation_service_.generate(gen_request);
    if (!gen_result.ok) {
        item.status = WorldRegionEnsureStatus::GenerationFailed;
        item.failure_reason_key = toString(gen_result.failure_kind);
        return item;
    }

    // Apply to runtime
    WorldGenerationApplier applier(world_runtime_, location_port_);
    auto apply_result = applier.apply(gen_result);
    if (!apply_result.ok) {
        item.status = WorldRegionEnsureStatus::ApplyFailed;
        item.failure_reason_key = toString(apply_result.failure_kind);
        return item;
    }

    item.status = WorldRegionEnsureStatus::GeneratedAndApplied;
    item.generated_cell_count = static_cast<int>(gen_result.cell_drafts.size());
    item.generated_entity_count = static_cast<int>(gen_result.entity_drafts.size());
    item.generated_resource_node_count = static_cast<int>(gen_result.resource_node_drafts.size());
    item.changed_cell_ids = apply_result.changed_cell_ids;
    item.changed_entity_ids = apply_result.changed_entity_ids;

    return item;
}

Result<WorldRegionEnsureResult> WorldRegionEnsureService::ensureRegions(
    const WorldRegionEnsureRequest& request) {

    auto valid = request.validateBasic();
    if (valid.is_error()) {
        return Result<WorldRegionEnsureResult>::fail(valid.errors());
    }

    WorldRegionEnsureResult result;
    result.ok = true;
    result.ensure_kind = request.ensure_kind;
    result.coverage_mode = request.coverage_mode;

    // Build request_id
    std::ostringstream oss;
    oss << "ensure_" << toString(request.ensure_kind) << "_" << request.world_id;
    result.request_id = oss.str();

    auto desired_regions = computeDesiredRegions(request);
    std::sort(desired_regions.begin(), desired_regions.end());
    auto last = std::unique(desired_regions.begin(), desired_regions.end());
    desired_regions.erase(last, desired_regions.end());

    int generated_so_far = 0;
    int missing_count = 0;

    // First pass: count missing regions
    for (const auto& region : desired_regions) {
        WorldRegionKey check_key;
        check_key.world_id = request.world_id;
        check_key.layer_key = request.layer_key;
        check_key.rx = region.rx;
        check_key.ry = region.ry;
        check_key.region_size = request.region_size;
        if (!world_runtime_.isRegionGenerated(check_key.regionRuntimeId())) {
            ++missing_count;
        }
    }

    if (missing_count > request.max_regions_to_generate && request.max_regions_to_generate > 0) {
        result.ok = false;
        WorldRegionEnsureItemResult item;
        item.status = WorldRegionEnsureStatus::LimitExceeded;
        item.failure_reason_key = "max_regions_exceeded:" + std::to_string(missing_count) + ">" + std::to_string(request.max_regions_to_generate);
        item.region_key.world_id = request.world_id;
        item.region_key.layer_key = request.layer_key;
        item.region_key.region_size = request.region_size;
        result.item_results.push_back(std::move(item));
        result.failure_reason_keys.push_back(item.failure_reason_key);
        return Result<WorldRegionEnsureResult>::ok(std::move(result));
    }

    for (const auto& region : desired_regions) {
        auto item = ensureSingleRegion(request, region);
        result.item_results.push_back(item);

        if (item.status == WorldRegionEnsureStatus::GeneratedAndApplied) {
            ++result.generated_region_count;
            ++generated_so_far;
            result.changed_cell_ids.insert(result.changed_cell_ids.end(), item.changed_cell_ids.begin(), item.changed_cell_ids.end());
            result.changed_entity_ids.insert(result.changed_entity_ids.end(), item.changed_entity_ids.begin(), item.changed_entity_ids.end());
        } else if (item.status == WorldRegionEnsureStatus::AlreadyAvailable) {
            ++result.already_available_count;
        } else if (item.status == WorldRegionEnsureStatus::GenerationFailed ||
                   item.status == WorldRegionEnsureStatus::ApplyFailed ||
                   item.status == WorldRegionEnsureStatus::InvalidRequest) {
            result.ok = false;
            if (!item.failure_reason_key.empty()) {
                result.failure_reason_keys.push_back(item.failure_reason_key);
            }
        }

        if (!item.warning_keys.empty()) {
            result.warning_keys.insert(result.warning_keys.end(), item.warning_keys.begin(), item.warning_keys.end());
        }
    }

    // Deduplicate changed ids
    std::sort(result.changed_cell_ids.begin(), result.changed_cell_ids.end());
    result.changed_cell_ids.erase(std::unique(result.changed_cell_ids.begin(), result.changed_cell_ids.end()), result.changed_cell_ids.end());
    std::sort(result.changed_entity_ids.begin(), result.changed_entity_ids.end());
    result.changed_entity_ids.erase(std::unique(result.changed_entity_ids.begin(), result.changed_entity_ids.end()), result.changed_entity_ids.end());

    return Result<WorldRegionEnsureResult>::ok(std::move(result));
}

} // namespace pathfinder::world_generation
