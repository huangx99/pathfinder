#include <cassert>
#include <iostream>
#include <vector>
#include "pathfinder/world_generation/world_region_math.h"

using namespace pathfinder::world_generation;
using pathfinder::world_runtime::WorldCellCoord;

void run_floor_div_tests() {
    assert(WorldRegionMath::floorDiv(15, 16) == 0);
    assert(WorldRegionMath::floorDiv(16, 16) == 1);
    assert(WorldRegionMath::floorDiv(-1, 16) == -1);
    assert(WorldRegionMath::floorDiv(-16, 16) == -1);
    assert(WorldRegionMath::floorDiv(-17, 16) == -2);
    assert(WorldRegionMath::floorDiv(0, 16) == 0);
    assert(WorldRegionMath::floorDiv(7, 16) == 0);
    assert(WorldRegionMath::floorDiv(8, 16) == 0);
    assert(WorldRegionMath::floorDiv(-8, 16) == -1);
    assert(WorldRegionMath::floorDiv(-9, 16) == -1);
    std::cout << "world_region_math_floor_div: passed" << std::endl;
}

void run_coord_to_region_tests() {
    int rs = 16;
    // region (0,0) covers x=-8..7, y=-8..7
    auto r00 = WorldRegionMath::coordToRegion(0, 0, rs);
    assert(r00.rx == 0 && r00.ry == 0);

    auto r00_b = WorldRegionMath::coordToRegion(-8, -8, rs);
    assert(r00_b.rx == 0 && r00_b.ry == 0);

    auto r00_c = WorldRegionMath::coordToRegion(7, 7, rs);
    assert(r00_c.rx == 0 && r00_c.ry == 0);

    auto r10 = WorldRegionMath::coordToRegion(8, 0, rs);
    assert(r10.rx == 1 && r10.ry == 0);

    auto r01 = WorldRegionMath::coordToRegion(0, 8, rs);
    assert(r01.rx == 0 && r01.ry == 1);

    auto r_m10 = WorldRegionMath::coordToRegion(-9, 0, rs);
    assert(r_m10.rx == -1 && r_m10.ry == 0);

    auto r_0m1 = WorldRegionMath::coordToRegion(0, -9, rs);
    assert(r_0m1.rx == 0 && r_0m1.ry == -1);

    auto r_m1m1 = WorldRegionMath::coordToRegion(-9, -9, rs);
    assert(r_m1m1.rx == -1 && r_m1m1.ry == -1);

    // Edge cases from design doc
    // region (0,0): -8..7, (-1,0): -24..-9, (1,0): 8..23, (2,0): 24..39
    assert(WorldRegionMath::coordToRegion(-25, 0, rs).rx == -2);
    assert(WorldRegionMath::coordToRegion(-24, 0, rs).rx == -1);
    assert(WorldRegionMath::coordToRegion(-9, 0, rs).rx == -1);
    assert(WorldRegionMath::coordToRegion(-8, 0, rs).rx == 0);
    assert(WorldRegionMath::coordToRegion(-1, 0, rs).rx == 0);
    assert(WorldRegionMath::coordToRegion(7, 0, rs).rx == 0);
    assert(WorldRegionMath::coordToRegion(8, 0, rs).rx == 1);
    assert(WorldRegionMath::coordToRegion(23, 0, rs).rx == 1);
    assert(WorldRegionMath::coordToRegion(24, 0, rs).rx == 2);

    std::cout << "world_region_math_coord_to_region: passed" << std::endl;
}

void run_regions_covering_coords_tests() {
    std::vector<WorldCellCoord> coords;
    coords.push_back(WorldCellCoord{0, 0, "surface"});
    coords.push_back(WorldCellCoord{8, 0, "surface"});
    coords.push_back(WorldCellCoord{-9, 0, "surface"});

    auto regions = WorldRegionMath::regionsCoveringCoords(coords, 16);
    assert(regions.size() == 3);
    // Should contain (0,0), (1,0), (-1,0)
    bool has_r00 = false, has_r10 = false, has_rm10 = false;
    for (const auto& r : regions) {
        if (r.rx == 0 && r.ry == 0) has_r00 = true;
        if (r.rx == 1 && r.ry == 0) has_r10 = true;
        if (r.rx == -1 && r.ry == 0) has_rm10 = true;
    }
    assert(has_r00 && has_r10 && has_rm10);
    std::cout << "world_region_math_regions_covering_coords: passed" << std::endl;
}

void run_regions_covering_rect_tests() {
    auto regions = WorldRegionMath::regionsCoveringRect(-10, 10, -10, 10, 16);
    // Should cover (-1,-1), (-1,0), (0,-1), (0,0), (1,0), (0,1), (1,1), (-1,1), (1,-1)
    assert(regions.size() == 9);
    std::cout << "world_region_math_regions_covering_rect: passed" << std::endl;
}

void run_cells_in_region_tests() {
    auto cells = WorldRegionMath::cellsInRegion(WorldRegionCoord{0, 0}, 16, "surface");
    assert(cells.size() == 256); // 16x16
    // Check bounds
    for (const auto& c : cells) {
        assert(c.x >= -8 && c.x <= 7);
        assert(c.y >= -8 && c.y <= 7);
        assert(c.layer_key == "surface");
    }
    std::cout << "world_region_math_cells_in_region: passed" << std::endl;
}

void run_no_overlap_tests() {
    // Adjacent regions should have no overlapping cells
    auto r0 = WorldRegionMath::cellsInRegion(WorldRegionCoord{0, 0}, 16, "surface");
    auto r1 = WorldRegionMath::cellsInRegion(WorldRegionCoord{1, 0}, 16, "surface");
    for (const auto& c0 : r0) {
        for (const auto& c1 : r1) {
            assert(!(c0.x == c1.x && c0.y == c1.y));
        }
    }
    std::cout << "world_region_math_no_overlap: passed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_floor_div_tests();
        run_coord_to_region_tests();
        run_regions_covering_coords_tests();
        run_regions_covering_rect_tests();
        run_cells_in_region_tests();
        run_no_overlap_tests();
        return 0;
    }
    std::string test = argv[1];
    if (test == "floor_div") run_floor_div_tests();
    else if (test == "coord_to_region") run_coord_to_region_tests();
    else if (test == "covering_coords") run_regions_covering_coords_tests();
    else if (test == "covering_rect") run_regions_covering_rect_tests();
    else if (test == "cells_in_region") run_cells_in_region_tests();
    else if (test == "no_overlap") run_no_overlap_tests();
    else {
        std::cerr << "Unknown test: " << test << std::endl;
        return 1;
    }
    return 0;
}
