#include "world/SandboxMapLayer.h"
#include "procedural/ProceduralArt.h"
#include "ui/UiStyle.h"

#include <unordered_map>

namespace pf::world {
namespace {
constexpr float kTileSize = 44.0F;

ax::Node* createTerrainArt(const std::string& key) {
    if (key == "water") return pf::art::createWaterTile(kTileSize);
    if (key == "soil") return pf::art::createSoilTile(kTileSize);
    return pf::art::createGrassTile(kTileSize);
}

ax::Node* createObjectArt(const std::string& key) {
    if (key == "red_berry") return pf::art::createRedBerry(kTileSize * 0.78F);
    if (key == "decayed_red_berry") return pf::art::createDecayedRedBerry(kTileSize * 0.78F);
    if (key == "stone_flake") return pf::art::createStoneFlake(kTileSize * 0.74F);
    if (key == "bitter_leaf") return pf::art::createBitterLeaf(kTileSize * 0.74F);
    if (key == "wood") return pf::art::createWood(kTileSize * 0.74F);
    return pf::art::createStoneFlake(kTileSize * 0.70F);
}

const pathfinder::v3_sandbox::V3ObjectInstanceView* findObject(
    const pathfinder::v3_sandbox::V3SandboxSnapshot& snapshot,
    const std::string& id) {
    for (const auto& object : snapshot.objects) {
        if (object.instance_id == id) return &object;
    }
    return nullptr;
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
        return positionToCell(convertToNodeSpace(touch->getLocation()), x, y);
    };
    listener->onTouchEnded = [this](ax::Touch* touch, ax::Event*) {
        int x = 0;
        int y = 0;
        if (positionToCell(convertToNodeSpace(touch->getLocation()), x, y) && on_cell_clicked_) on_cell_clicked_(x, y);
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    return true;
}

void SandboxMapLayer::render(const pathfinder::v3_sandbox::V3SandboxSnapshot& snapshot, int selected_x, int selected_y) {
    removeAllChildren();
    map_width_ = snapshot.width;
    map_height_ = snapshot.height;
    setContentSize(ax::Size(snapshot.width * kTileSize, snapshot.height * kTileSize));

    auto* background = ax::DrawNode::create();
    background->drawSolidRect(ax::Vec2::ZERO, ax::Vec2(getContentSize().width, getContentSize().height), pf::ui::color(7, 13, 23));
    addChild(background, 0);

    for (const auto& cell : snapshot.cells) {
        auto* tile = createTerrainArt(cell.terrain_key);
        tile->setPosition(cellToPosition(cell.x, cell.y, snapshot.height));
        addChild(tile, 1);
    }

    auto* grid = ax::DrawNode::create();
    for (int x = 0; x <= snapshot.width; ++x) {
        const float px = x * kTileSize;
        grid->drawLine(ax::Vec2(px, 0), ax::Vec2(px, snapshot.height * kTileSize), pf::ui::color(15, 23, 42, 0.25F));
    }
    for (int y = 0; y <= snapshot.height; ++y) {
        const float py = y * kTileSize;
        grid->drawLine(ax::Vec2(0, py), ax::Vec2(snapshot.width * kTileSize, py), pf::ui::color(15, 23, 42, 0.25F));
    }
    addChild(grid, 2);

    for (const auto& cell : snapshot.cells) {
        const auto center = cellToPosition(cell.x, cell.y, snapshot.height);
        for (const auto& object_id : cell.object_instance_ids) {
            if (const auto* object = findObject(snapshot, object_id)) {
                auto* art = createObjectArt(object->object_key);
                art->setPosition(center + ax::Vec2(-5.0F, -3.0F));
                addChild(art, 4);
            }
        }
        for (const auto& agent_id : cell.agent_ids) {
            auto* actor = pf::art::createPlayer(kTileSize * 0.86F);
            actor->setPosition(center + ax::Vec2(4.0F, 2.0F));
            addChild(actor, 6);
        }
    }

    if (selected_x >= 0 && selected_y >= 0) {
        auto* selected = pf::art::createSelectionTile(kTileSize);
        selected->setPosition(cellToPosition(selected_x, selected_y, snapshot.height));
        addChild(selected, 8);
    }
}

ax::Vec2 SandboxMapLayer::cellToPosition(int x, int y, int map_height) const {
    return ax::Vec2((static_cast<float>(x) + 0.5F) * kTileSize, (static_cast<float>(map_height - 1 - y) + 0.5F) * kTileSize);
}

bool SandboxMapLayer::positionToCell(const ax::Vec2& local, int& x, int& y) const {
    if (map_width_ <= 0 || map_height_ <= 0) return false;
    if (local.x < 0.0F || local.y < 0.0F || local.x >= map_width_ * kTileSize || local.y >= map_height_ * kTileSize) return false;
    x = static_cast<int>(local.x / kTileSize);
    y = map_height_ - 1 - static_cast<int>(local.y / kTileSize);
    return true;
}

} // namespace pf::world
