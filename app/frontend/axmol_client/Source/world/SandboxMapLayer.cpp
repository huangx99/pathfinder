#include "world/SandboxMapLayer.h"
#include "procedural/ProceduralArt.h"
#include "ui/PixelUI.h"
#include "pathfinder/logging/logger.h"

#include <algorithm>

namespace pf::world {
namespace {

constexpr float kTileSize = 30.0F;

ax::Node* createTerrainArt(const std::string& key) {
    if (key == "water_edge" || key == "deep_water") return pf::art::createWaterTile(kTileSize);
    if (key == "stone_field" || key == "mountain" || key == "blocked") return pf::art::createSoilTile(kTileSize);
    return pf::art::createGrassTile(kTileSize);
}

ax::Node* createObjectArt(const std::string& key) {
    if (key == "red_berry" || key == "berry_bush") return pf::art::createRedBerry(kTileSize * 0.78F);
    if (key == "decayed_red_berry") return pf::art::createDecayedRedBerry(kTileSize * 0.78F);
    if (key == "bitter_leaf") return pf::art::createBitterLeaf(kTileSize * 0.74F);
    if (key == "wood" || key == "young_tree") return pf::art::createWood(kTileSize * 0.74F);
    return pf::art::createStoneFlake(kTileSize * 0.70F);
}

} // namespace

SandboxMapLayer* SandboxMapLayer::create(std::function<void(int, int)> on_cell_clicked) {
    auto* layer = new SandboxMapLayer();
    if (layer && layer->init(std::move(on_cell_clicked))) {
        layer->autorelease();
        return layer;
    }
    delete layer;
    return nullptr;
}

bool SandboxMapLayer::init(std::function<void(int, int)> on_cell_clicked) {
    if (!Node::init()) return false;
    on_cell_clicked_ = std::move(on_cell_clicked);
    auto* listener = ax::EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [this](ax::Touch* touch, ax::Event*) {
        int x = 0;
        int y = 0;
        const bool hit = positionToCell(convertToNodeSpace(touch->getLocation()), x, y);
        if (hit) {
            pathfinder::logging::log(pathfinder::logging::tag::Input, "map touch began x=" + std::to_string(x) + " y=" + std::to_string(y));
        }
        return hit;
    };
    listener->onTouchEnded = [this](ax::Touch* touch, ax::Event*) {
        int x = 0;
        int y = 0;
        if (positionToCell(convertToNodeSpace(touch->getLocation()), x, y) && on_cell_clicked_) {
            pathfinder::logging::log(pathfinder::logging::tag::Input, "map touch ended x=" + std::to_string(x) + " y=" + std::to_string(y));
            on_cell_clicked_(x, y);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    return true;
}

void SandboxMapLayer::render(const pf::client::EngineSnapshot& snapshot, int selected_x, int selected_y) {
    map_width_ = snapshot.width;
    map_height_ = snapshot.height;
    min_x_ = snapshot.min_x;
    min_y_ = snapshot.min_y;
    setContentSize(ax::Size(snapshot.width * kTileSize, snapshot.height * kTileSize));

    const bool first_time = (terrain_layer_ == nullptr);
    const bool size_changed = (cached_width_ != snapshot.width || cached_height_ != snapshot.height);

    if (first_time || size_changed) {
        pathfinder::logging::log(pathfinder::logging::tag::Map, "map layer rebuilt width=" + std::to_string(snapshot.width) + " height=" + std::to_string(snapshot.height) + " min_x=" + std::to_string(snapshot.min_x) + " min_y=" + std::to_string(snapshot.min_y));
        removeAllChildren();
        cache_.clear();
        cache_.resize(std::max(0, snapshot.width * snapshot.height));
        cached_width_ = snapshot.width;
        cached_height_ = snapshot.height;

        auto* bg = ax::DrawNode::create();
        bg->drawSolidRect(ax::Vec2::ZERO,
                          ax::Vec2(getContentSize().width, getContentSize().height),
                          pf::ui::pixelColor(24, 58, 38));
        addChild(bg, 0);

        terrain_layer_ = ax::Node::create();
        addChild(terrain_layer_, 1);

        grid_layer_ = ax::DrawNode::create();
        for (int x = 0; x <= snapshot.width; ++x) {
            const float px = x * kTileSize;
            grid_layer_->drawLine(ax::Vec2(px, 0), ax::Vec2(px, snapshot.height * kTileSize),
                                  pf::ui::pixelColor(15, 23, 42, 0.25F));
        }
        for (int y = 0; y <= snapshot.height; ++y) {
            const float py = y * kTileSize;
            grid_layer_->drawLine(ax::Vec2(0, py), ax::Vec2(snapshot.width * kTileSize, py),
                                  pf::ui::pixelColor(15, 23, 42, 0.25F));
        }
        addChild(grid_layer_, 2);

        object_layer_ = ax::Node::create();
        addChild(object_layer_, 4);
        agent_layer_ = ax::Node::create();
        addChild(agent_layer_, 6);
        selection_layer_ = ax::Node::create();
        addChild(selection_layer_, 8);
    } else {
        object_layer_->removeAllChildren();
        agent_layer_->removeAllChildren();
    }

    for (const auto& cell : snapshot.cells) {
        const int local_x = cell.x - snapshot.min_x;
        const int local_y = cell.y - snapshot.min_y;
        const int index = local_y * snapshot.width + local_x;
        if (index < 0 || index >= static_cast<int>(cache_.size())) continue;

        auto& cached = cache_[index];
        if (cached.terrain_key != cell.terrain_key) {
            cached.terrain_key = cell.terrain_key;
            auto* old = terrain_layer_->getChildByName("t_" + std::to_string(index));
            if (old) old->removeFromParent();

            auto* tile = createTerrainArt(cell.terrain_key);
            tile->setName("t_" + std::to_string(index));
            tile->setPosition(cellToPosition(cell.x, cell.y, snapshot));
            terrain_layer_->addChild(tile, 1);
        }
        cached.entity_ids = cell.entity_ids;
    }

    for (const auto& entity : snapshot.entities) {
        if (entity.entity_key == "player" || !entity.actor_key.empty()) continue;
        auto* art = createObjectArt(entity.entity_key);
        art->setPosition(cellToPosition(entity.x, entity.y, snapshot) + ax::Vec2(-5.0F, -3.0F));
        object_layer_->addChild(art, 4);
    }

    for (const auto& agent : snapshot.agents) {
        auto* actor = pf::art::createPlayer(kTileSize * 0.86F);
        actor->setPosition(cellToPosition(agent.x, agent.y, snapshot) + ax::Vec2(4.0F, 2.0F));
        agent_layer_->addChild(actor, 6);
    }

    selection_layer_->removeAllChildren();
    if (selected_x >= snapshot.min_x && selected_y >= snapshot.min_y &&
        selected_x < snapshot.min_x + snapshot.width && selected_y < snapshot.min_y + snapshot.height) {
        auto* selected = pf::art::createSelectionTile(kTileSize);
        selected->setPosition(cellToPosition(selected_x, selected_y, snapshot));
        selection_layer_->addChild(selected, 8);
    }
}

ax::Vec2 SandboxMapLayer::cellToPosition(int x, int y, const pf::client::EngineSnapshot& snapshot) const {
    const int local_x = x - snapshot.min_x;
    const int local_y = y - snapshot.min_y;
    return ax::Vec2((static_cast<float>(local_x) + 0.5F) * kTileSize,
                    (static_cast<float>(snapshot.height - 1 - local_y) + 0.5F) * kTileSize);
}

bool SandboxMapLayer::positionToCell(const ax::Vec2& local, int& x, int& y) const {
    if (map_width_ <= 0 || map_height_ <= 0) return false;
    if (local.x < 0.0F || local.y < 0.0F ||
        local.x >= map_width_ * kTileSize || local.y >= map_height_ * kTileSize) {
        return false;
    }
    x = min_x_ + static_cast<int>(local.x / kTileSize);
    y = min_y_ + map_height_ - 1 - static_cast<int>(local.y / kTileSize);
    return true;
}

} // namespace pf::world
