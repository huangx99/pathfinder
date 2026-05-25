#pragma once

#include "axmol/axmol.h"
#include "runtime/EngineLocalClient.h"
#include <functional>
#include <string>
#include <vector>

namespace pf::world {

class SandboxMapLayer final : public ax::Node {
public:
    static SandboxMapLayer* create(std::function<void(int, int)> on_cell_clicked);
    bool init(std::function<void(int, int)> on_cell_clicked);
    void render(const pf::client::EngineSnapshot& snapshot, int selected_x, int selected_y);

private:
    ax::Vec2 cellToPosition(int x, int y, const pf::client::EngineSnapshot& snapshot) const;
    bool positionToCell(const ax::Vec2& local, int& x, int& y) const;

    struct CachedCell {
        std::string terrain_key;
        std::vector<std::string> entity_ids;
    };

    std::function<void(int, int)> on_cell_clicked_;
    std::vector<CachedCell> cache_;
    int cached_width_{0};
    int cached_height_{0};
    int map_width_{0};
    int map_height_{0};
    int min_x_{0};
    int min_y_{0};

    ax::Node* terrain_layer_{nullptr};
    ax::Node* object_layer_{nullptr};
    ax::Node* agent_layer_{nullptr};
    ax::DrawNode* grid_layer_{nullptr};
    ax::Node* selection_layer_{nullptr};
};

} // namespace pf::world
