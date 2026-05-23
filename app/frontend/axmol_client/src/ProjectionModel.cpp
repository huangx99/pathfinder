#include "ProjectionModel.h"

namespace pathfinder::axmol_client {

void ProjectionModel::clear() {
    cells_.clear();
    entities_.clear();
}

void ProjectionModel::setCells(std::vector<CellView> cells) {
    cells_ = std::move(cells);
}

void ProjectionModel::setEntities(std::vector<EntityView> entities) {
    entities_ = std::move(entities);
}

const std::vector<CellView>& ProjectionModel::cells() const {
    return cells_;
}

const std::vector<EntityView>& ProjectionModel::entities() const {
    return entities_;
}

} // namespace pathfinder::axmol_client
