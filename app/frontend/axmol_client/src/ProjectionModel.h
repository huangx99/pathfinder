#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace pathfinder::axmol_client {

struct CellView {
    std::string cell_id;
    int x{0};
    int y{0};
    std::string layer_key{"surface"};
    std::string terrain_key{"unknown"};
    std::string visibility{"Unknown"};
    bool blocks_movement{false};
};

struct EntityView {
    std::string entity_id;
    std::string entity_key;
    int x{0};
    int y{0};
    std::string layer_key{"surface"};
};

class ProjectionModel {
public:
    void clear();
    void setCells(std::vector<CellView> cells);
    void setEntities(std::vector<EntityView> entities);

    const std::vector<CellView>& cells() const;
    const std::vector<EntityView>& entities() const;

private:
    std::vector<CellView> cells_;
    std::vector<EntityView> entities_;
};

} // namespace pathfinder::axmol_client
