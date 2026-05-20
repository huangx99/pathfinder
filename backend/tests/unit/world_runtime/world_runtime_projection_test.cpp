#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_runtime;

void run_projection_cell_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 5;
    config.initial_region_radius = 0;
    config.default_vision_radius = 2;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldProjectionAdapter adapter;

    // Get visible cells
    auto visible = runtime.getVisibleCellIds("player");
    assert(!visible.empty());

    auto patches_res = adapter.buildCellPatches(visible, "player", runtime);
    assert(patches_res.is_ok());
    auto patches = patches_res.value();
    assert(!patches.empty());

    // All patches should have coordinates
    for (const auto& patch : patches) {
        assert(!patch.cell_id.empty());
        assert(patch.fields.count("x"));
        assert(patch.fields.count("y"));
        assert(patch.fields.count("layer_key"));
        assert(patch.fields.count("terrain_key"));
    }

    std::cout << "world_runtime_projection_cell: passed" << std::endl;
}

void run_projection_entity_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 5;
    config.initial_region_radius = 0;
    config.default_vision_radius = 2;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldProjectionAdapter adapter;

    auto snap = runtime.snapshotForDebug();
    assert(snap.is_ok());

    std::vector<std::string> entity_ids;
    for (const auto& [id, entity] : snap.value().entities) {
        entity_ids.push_back(id);
    }

    auto patches_res = adapter.buildEntityPatches(entity_ids, "player", runtime);
    assert(patches_res.is_ok());
    auto patches = patches_res.value();

    // Entities in visible cells should be present
    for (const auto& patch : patches) {
        assert(!patch.entity_id.empty());
        assert(patch.fields.count("entity_key"));
        assert(patch.fields.count("x"));
        assert(patch.fields.count("y"));
    }

    std::cout << "world_runtime_projection_entity: passed" << std::endl;
}

void run_projection_no_unknown_cells_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 5;
    config.initial_region_radius = 0;
    config.default_vision_radius = 1;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldProjectionAdapter adapter;

    // Try to patch all cells including unknown ones
    std::vector<std::string> all_cell_ids;
    auto snap = runtime.snapshotForDebug();
    for (const auto& [id, cell] : snap.value().cells) {
        all_cell_ids.push_back(id);
    }

    auto patches_res = adapter.buildCellPatches(all_cell_ids, "player", runtime);
    assert(patches_res.is_ok());
    auto patches = patches_res.value();

    // Unknown cells should be filtered out
    for (const auto& patch : patches) {
        auto vis = runtime.getCellVisibility("player", patch.cell_id);
        assert(vis != WorldCellVisibility::Unknown);
    }

    std::cout << "world_projection_no_hidden_unknown_cells: passed" << std::endl;
}
