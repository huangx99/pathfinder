#pragma once

#include "axmol/axmol.h"
#include "pathfinder/v3_sandbox/v3_sandbox_types.h"
#include <functional>

namespace pf::world {

class SandboxMapLayer final : public ax::Node {
public:
    static SandboxMapLayer* create(std::function<void(int, int)> on_cell_clicked);
    bool init(std::function<void(int, int)> on_cell_clicked);
    void render(const pathfinder::v3_sandbox::V3SandboxSnapshot& snapshot, int selected_x, int selected_y);

private:
    ax::Vec2 cellToPosition(int x, int y, int map_height) const;
    bool positionToCell(const ax::Vec2& local, int& x, int& y) const;

    std::function<void(int, int)> on_cell_clicked_;
    int map_width_{0};
    int map_height_{0};
};

} // namespace pf::world
