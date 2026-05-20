#include "pathfinder/world_runtime/world_runtime_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_runtime;

void run_coordinate_id_tests() {
    WorldCellCoord coord{1, 2, "surface"};
    assert(coord.cellId() == "surface:1:2");

    WorldCellCoord coord2{-3, 4, "underground"};
    assert(coord2.cellId() == "underground:-3:4");

    WorldCellCoord coord3{0, 0, "surface"};
    assert(coord3.cellId() == "surface:0:0");

    std::cout << "world_runtime_coordinate_id: passed" << std::endl;
}

void run_coordinate_comparison_tests() {
    WorldCellCoord a{1, 2, "surface"};
    WorldCellCoord b{1, 2, "surface"};
    WorldCellCoord c{2, 2, "surface"};
    WorldCellCoord d{1, 2, "underground"};

    assert(a == b);
    assert(!(a == c));
    assert(!(a == d));
    assert(a < c);
    assert(a < d); // surface < underground

    std::cout << "world_runtime_coordinate_comparison: passed" << std::endl;
}

void run_enum_tests() {
    assert(toString(WorldLayerPolicy::SurfaceOnly) == "SurfaceOnly");
    assert(toString(WorldLayerPolicy::ExplicitLayerOnly) == "ExplicitLayerOnly");
    assert(toString(WorldLayerPolicy::AllowLayerSpawn) == "AllowLayerSpawn");

    assert(toString(WorldCellVisibility::Unknown) == "Unknown");
    assert(toString(WorldCellVisibility::Discovered) == "Discovered");
    assert(toString(WorldCellVisibility::Visible) == "Visible");

    assert(toString(WorldEntityLocationKind::Nowhere) == "Nowhere");
    assert(toString(WorldEntityLocationKind::OnMap) == "OnMap");
    assert(toString(WorldEntityLocationKind::InInventory) == "InInventory");
    assert(toString(WorldEntityLocationKind::InContainer) == "InContainer");
    assert(toString(WorldEntityLocationKind::Equipped) == "Equipped");

    assert(toString(WorldMoveBlockReason::None) == "None");
    assert(toString(WorldMoveBlockReason::OutOfBounds) == "OutOfBounds");
    assert(toString(WorldMoveBlockReason::NotAdjacent) == "NotAdjacent");
    assert(toString(WorldMoveBlockReason::CellBlocked) == "CellBlocked");

    std::cout << "world_runtime_enum_tests: passed" << std::endl;
}
