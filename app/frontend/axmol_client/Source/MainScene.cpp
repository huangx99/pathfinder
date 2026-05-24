#include "MainScene.h"
#include "procedural/ProceduralArt.h"
#include "ui/ClientUiFormat.h"
#include "pathfinder/world_command/world_command_types.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <vector>

using namespace ax;

namespace {
constexpr float kTileSize = 48.0F;
constexpr float kMinZoom = 0.75F;
constexpr float kMaxZoom = 2.0F;
constexpr double kDoubleClickSeconds = 0.32;
constexpr float kPathStepSeconds = 0.16F;
constexpr float kPlayerMoveTweenSeconds = 0.14F;
constexpr float kWorldAutoTickSeconds = 0.10F;
constexpr int kEntityMoveActionTag = 6101;
constexpr int kRenderMarginTiles = 4;
constexpr float kInventorySlotSize = 42.0F;
constexpr float kInventorySlotGap = 4.0F;
constexpr int kInventoryHotbarSlots = 9;
constexpr int kHotbarItemSlots = 8;

Color32 bgColor() { return Color32(9, 20, 35, 255); }
Color gridColor() { return Color(15 / 255.0F, 23 / 255.0F, 42 / 255.0F, 0.18F); }

Color interactColor() { return Color(250 / 255.0F, 204 / 255.0F, 21 / 255.0F, 0.66F); }


std::string inventoryItemKey(const pf::client::LocalInventoryItemView& item) {
    if (!item.entry_id.empty()) return "entry:" + item.entry_id;
    if (!item.entity_id.empty()) return "entity:" + item.entity_id;
    return "stack:" + item.stack_key + ":" + item.entity_key;
}

int findInventoryIndexByKey(const std::vector<pf::client::LocalInventoryItemView>& items, const std::string& key) {
    if (key.empty()) return -1;
    for (size_t index = 0; index < items.size(); ++index) {
        if (inventoryItemKey(items[index]) == key) return static_cast<int>(index);
    }
    return -1;
}

double inventoryNumericState(const pf::client::LocalInventoryItemView& item, const std::string& key, double fallback = 0.0) {
    auto it = item.numeric_states.find(key);
    return it == item.numeric_states.end() ? fallback : it->second;
}

bool hasDurabilityState(const pf::client::LocalInventoryItemView& item) {
    return item.numeric_states.find("durability") != item.numeric_states.end() &&
           item.numeric_states.find("max_durability") != item.numeric_states.end();
}

void addDurabilityBar(Node* parent, const pf::client::LocalInventoryItemView& item, const Rect& rect, int z) {
    if (!parent || !hasDurabilityState(item)) return;
    const double durability = std::max(0.0, inventoryNumericState(item, "durability"));
    const double max_durability = std::max(1.0, inventoryNumericState(item, "max_durability", durability));
    const float ratio = std::clamp(static_cast<float>(durability / max_durability), 0.0F, 1.0F);
    auto* bar = DrawNode::create();
    const float left = rect.getMinX() + 5.0F;
    const float right = rect.getMaxX() - 5.0F;
    const float bottom = rect.getMinY() + 4.0F;
    const float top = bottom + 4.0F;
    bar->drawSolidRect(Vec2(left, bottom), Vec2(right, top), Color(15 / 255.0F, 23 / 255.0F, 42 / 255.0F, 0.92F));
    bar->drawSolidRect(Vec2(left, bottom), Vec2(left + (right - left) * ratio, top), Color(34 / 255.0F, 197 / 255.0F, 94 / 255.0F, 0.95F));
    parent->addChild(bar, z);
}

std::string uiFontPath() {
    static const std::array<std::string, 7> candidates{
        "fonts/NotoSansCJK-Regular.ttc",
        "fonts/NotoSansSC-Regular.otf",
        "fonts/SourceHanSansSC-Regular.otf",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/arphic/uming.ttc",
    };

    auto* file_utils = FileUtils::getInstance();
    for (const auto& path : candidates) {
        if (file_utils->isFileExist(path)) return path;
    }
    return {};
}

Label* createUiLabel(float font_size, const Vec2& dimensions = Vec2::ZERO) {
    const auto font_path = uiFontPath();
    if (!font_path.empty()) {
        if (auto* label = Label::createWithTTF("", font_path, font_size, dimensions, TextHAlignment::LEFT, TextVAlignment::TOP)) {
            return label;
        }
    }
    return Label::createWithSystemFont("", "Noto Sans CJK SC", font_size, dimensions, TextHAlignment::LEFT, TextVAlignment::TOP);
}

std::string commandKindLabel(pathfinder::world_command::WorldCommandKind kind) {
    using pathfinder::world_command::WorldCommandKind;
    switch (kind) {
        case WorldCommandKind::Move: return "移动";
        case WorldCommandKind::Inspect: return "查看";
        case WorldCommandKind::Gather: return "采集";
        case WorldCommandKind::Chop: return "砍伐";
        case WorldCommandKind::Mine: return "采矿";
        case WorldCommandKind::Dig: return "挖掘";
        case WorldCommandKind::Pickup: return "拾取";
        case WorldCommandKind::Drop: return "放下";
        case WorldCommandKind::Craft: return "制作";
        case WorldCommandKind::Use: return "使用";
        case WorldCommandKind::Eat: return "食用";
        case WorldCommandKind::Teach: return "教学";
        case WorldCommandKind::Wait: return "等待";
        default: return "交互";
    }
}

Node* addAt(Node* parent, Node* child, const Vec2& position, int z) {
    if (!child) return nullptr;
    child->setPosition(position);
    parent->addChild(child, z);
    return child;
}

bool containsToken(const std::string& value, const std::string& token) {
    return value.find(token) != std::string::npos;
}

Color terrainBaseColor(const std::string& terrain) {
    if (containsToken(terrain, "water")) return Color(26 / 255.0F, 45 / 255.0F, 74 / 255.0F, 1.0F);
    if (containsToken(terrain, "forest")) return Color(15 / 255.0F, 46 / 255.0F, 28 / 255.0F, 1.0F);
    if (containsToken(terrain, "stone") || containsToken(terrain, "rock") || containsToken(terrain, "mountain")) return Color(76 / 255.0F, 82 / 255.0F, 94 / 255.0F, 1.0F);
    return Color(30 / 255.0F, 58 / 255.0F, 30 / 255.0F, 1.0F);
}

Color terrainDetailColor(const std::string& terrain, int variant) {
    if (containsToken(terrain, "water")) return variant % 2 == 0 ? Color(37 / 255.0F, 99 / 255.0F, 235 / 255.0F, 0.55F) : Color(59 / 255.0F, 130 / 255.0F, 246 / 255.0F, 0.45F);
    if (containsToken(terrain, "forest")) return variant % 2 == 0 ? Color(20 / 255.0F, 83 / 255.0F, 45 / 255.0F, 0.75F) : Color(22 / 255.0F, 101 / 255.0F, 52 / 255.0F, 0.65F);
    if (containsToken(terrain, "stone") || containsToken(terrain, "rock") || containsToken(terrain, "mountain")) return variant % 2 == 0 ? Color(107 / 255.0F, 114 / 255.0F, 128 / 255.0F, 0.65F) : Color(82 / 255.0F, 82 / 255.0F, 91 / 255.0F, 0.7F);
    return variant % 2 == 0 ? Color(20 / 255.0F, 83 / 255.0F, 45 / 255.0F, 0.58F) : Color(6 / 255.0F, 78 / 255.0F, 59 / 255.0F, 0.45F);
}

void drawRectAt(DrawNode* draw, const Vec2& center, float left, float top, float width, float height, const Color& color) {
    const float half = kTileSize * 0.5F;
    const Vec2 bottom_left(center.x - half + left, center.y + half - top - height);
    const Vec2 top_right(center.x - half + left + width, center.y + half - top);
    draw->drawSolidRect(bottom_left, top_right, color);
}

int terrainVariant(int x, int y) {
    return std::abs((x * 31) ^ (y * 17)) % 4;
}


int terrainNoise(int x, int y, int salt) {
    int value = x * 73856093 ^ y * 19349663 ^ salt * 83492791;
    value ^= value >> 13;
    value *= 1274126177;
    return std::abs(value);
}

bool isWaterTerrain(const std::string& terrain) {
    return containsToken(terrain, "water");
}

bool isForestTerrain(const std::string& terrain) {
    return containsToken(terrain, "forest");
}

bool isRockTerrain(const std::string& terrain) {
    return containsToken(terrain, "stone") || containsToken(terrain, "rock") || containsToken(terrain, "mountain");
}

void drawTerrainDetails(DrawNode* draw, const Vec2& center, const std::string& terrain, int x, int y) {
    const auto pick = [&](int salt, int modulo) {
        return terrainNoise(x, y, salt) % modulo;
    };
    const auto xf = [&](int salt, float min, float span) {
        return min + static_cast<float>(pick(salt, 1000)) * span / 999.0F;
    };
    const auto yf = [&](int salt, float min, float span) {
        return min + static_cast<float>(pick(salt, 1000)) * span / 999.0F;
    };

    if (isWaterTerrain(terrain)) {
        const auto deep = Color(30 / 255.0F, 64 / 255.0F, 175 / 255.0F, 0.24F);
        const auto blue = Color(37 / 255.0F, 99 / 255.0F, 235 / 255.0F, 0.30F);
        const auto light = Color(96 / 255.0F, 165 / 255.0F, 250 / 255.0F, 0.38F);
        const int wave_count = 2 + pick(21, 2);
        for (int index = 0; index < wave_count; ++index) {
            const float left = xf(30 + index * 7, 3.0F, 31.0F);
            const float top = yf(40 + index * 11, 6.0F, 33.0F);
            const float width = 7.0F + static_cast<float>(pick(50 + index * 13, 12));
            const auto color = (index % 3 == 0) ? light : (index % 3 == 1 ? blue : deep);
            drawRectAt(draw, center, left, top, width, 2.0F, color);
        }
        if (pick(70, 8) == 0) {
            drawRectAt(draw, center, xf(71, 8.0F, 27.0F), yf(72, 12.0F, 20.0F), 3.0F, 3.0F, Color(191 / 255.0F, 219 / 255.0F, 254 / 255.0F, 0.45F));
        }
        return;
    }

    if (isForestTerrain(terrain)) {
        const auto shadow = Color(5 / 255.0F, 46 / 255.0F, 22 / 255.0F, 0.30F);
        const auto leaf_dark = Color(6 / 255.0F, 78 / 255.0F, 59 / 255.0F, 0.40F);
        const auto leaf = Color(22 / 255.0F, 101 / 255.0F, 52 / 255.0F, 0.44F);
        const auto moss = Color(34 / 255.0F, 197 / 255.0F, 94 / 255.0F, 0.24F);
        const int patch_count = 2 + pick(101, 3);
        for (int index = 0; index < patch_count; ++index) {
            const float left = xf(110 + index * 17, 3.0F, 34.0F);
            const float top = yf(130 + index * 19, 4.0F, 34.0F);
            const float width = 5.0F + static_cast<float>(pick(150 + index * 23, 9));
            const float height = 3.0F + static_cast<float>(pick(170 + index * 29, 6));
            const auto color = (index % 4 == 0) ? shadow : (index % 4 == 1 ? leaf_dark : (index % 4 == 2 ? leaf : moss));
            drawRectAt(draw, center, left, top, width, height, color);
        }
        if (pick(190, 10) == 0) {
            drawRectAt(draw, center, xf(191, 8.0F, 28.0F), yf(192, 10.0F, 26.0F), 3.0F, 3.0F, Color(132 / 255.0F, 204 / 255.0F, 22 / 255.0F, 0.55F));
        }
        return;
    }

    if (isRockTerrain(terrain)) {
        const auto base_ridge = Color(107 / 255.0F, 114 / 255.0F, 128 / 255.0F, 0.40F);
        const auto crack = Color(31 / 255.0F, 41 / 255.0F, 55 / 255.0F, 0.24F);
        const auto highlight = Color(148 / 255.0F, 163 / 255.0F, 184 / 255.0F, 0.28F);
        const int stone_count = 2 + pick(211, 2);
        for (int index = 0; index < stone_count; ++index) {
            const float left = xf(220 + index * 31, 5.0F, 31.0F);
            const float top = yf(240 + index * 37, 7.0F, 30.0F);
            const float width = 5.0F + static_cast<float>(pick(260 + index * 41, 10));
            const float height = 2.0F + static_cast<float>(pick(280 + index * 43, 5));
            drawRectAt(draw, center, left, top, width, height, (index % 3 == 0) ? highlight : base_ridge);
        }
        const int crack_count = 1 + pick(301, 2);
        for (int index = 0; index < crack_count; ++index) {
            drawRectAt(draw, center, xf(310 + index * 47, 8.0F, 28.0F), yf(330 + index * 53, 10.0F, 28.0F), 4.0F + static_cast<float>(pick(350 + index * 59, 8)), 2.0F, crack);
        }
        return;
    }

    const auto grass_dark = Color(20 / 255.0F, 83 / 255.0F, 45 / 255.0F, 0.38F);
    const auto grass_mid = Color(22 / 255.0F, 101 / 255.0F, 52 / 255.0F, 0.34F);
    const auto grass_light = Color(74 / 255.0F, 222 / 255.0F, 128 / 255.0F, 0.22F);
    const int grass_count = 2 + pick(401, 3);
    for (int index = 0; index < grass_count; ++index) {
        const float left = xf(410 + index * 61, 3.0F, 37.0F);
        const float top = yf(430 + index * 67, 7.0F, 34.0F);
        const float width = 3.0F + static_cast<float>(pick(450 + index * 71, 8));
        const float height = 2.0F + static_cast<float>(pick(470 + index * 73, 5));
        const auto color = (index % 3 == 0) ? grass_light : (index % 3 == 1 ? grass_mid : grass_dark);
        drawRectAt(draw, center, left, top, width, height, color);
    }
    if (pick(501, 12) == 0) {
        drawRectAt(draw, center, xf(502, 5.0F, 35.0F), yf(503, 8.0F, 30.0F), 3.0F, 3.0F, Color(250 / 255.0F, 204 / 255.0F, 21 / 255.0F, 0.78F));
    }
    if (pick(511, 16) == 0) {
        drawRectAt(draw, center, xf(512, 5.0F, 35.0F), yf(513, 8.0F, 30.0F), 3.0F, 3.0F, Color(244 / 255.0F, 114 / 255.0F, 182 / 255.0F, 0.68F));
    }
}

} // namespace

bool MainScene::init() {
    if (!Scene::init()) return false;

    const auto visible_size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();
    world_origin_ = Vec2(origin.x + visible_size.width * 0.5F, origin.y + visible_size.height * 0.5F);

    auto background = LayerColor::create(bgColor());
    addChild(background, 0);

    world_layer_ = Node::create();
    terrain_layer_ = Node::create();
    water_layer_ = Node::create();
    path_layer_ = Node::create();
    entity_layer_ = Node::create();
    overlay_layer_ = Node::create();
    world_layer_->addChild(terrain_layer_, 0);
    world_layer_->addChild(water_layer_, 2);
    world_layer_->addChild(path_layer_, 5);
    world_layer_->addChild(entity_layer_, 10);
    world_layer_->addChild(overlay_layer_, 20);
    addChild(world_layer_, 1);

    ui_layer_ = Node::create();
    inventory_layer_ = Node::create();
    context_panel_layer_ = Node::create();
    ui_layer_->addChild(inventory_layer_, 1);
    ui_layer_->addChild(context_panel_layer_, 2);
    addChild(ui_layer_, 30);

    status_label_ = createUiLabel(15.0F);
    status_label_->setTextColor(Color32(148, 163, 184, 255));
    status_label_->setAnchorPoint(Vec2(0.0F, 1.0F));
    status_label_->setPosition(Vec2(origin.x + 14.0F, origin.y + visible_size.height - 14.0F));
    status_label_->setVisible(false);
    addChild(status_label_, 2);

    selection_label_ = createUiLabel(15.0F, Vec2(430.0F, 170.0F));
    selection_label_->setTextColor(Color32(226, 232, 240, 255));
    selection_label_->setAnchorPoint(Vec2(0.0F, 0.0F));
    selection_label_->setPosition(Vec2(origin.x + 14.0F, origin.y + 14.0F));
    selection_label_->setVisible(false);
    addChild(selection_label_, 2);

    auto mouse = EventListenerMouse::create();
    mouse->onMouseDown = AX_CALLBACK_1(MainScene::onMouseDown, this);
    mouse->onMouseUp = AX_CALLBACK_1(MainScene::onMouseUp, this);
    mouse->onMouseMove = AX_CALLBACK_1(MainScene::onMouseMove, this);
    mouse->onMouseScroll = AX_CALLBACK_1(MainScene::onMouseScroll, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouse, this);

    bootstrapLocalRuntime();
    scheduleUpdate();
    return true;
}

void MainScene::update(float delta) {
    world_layer_->setScale(zoom_);
    world_layer_->setPosition(world_origin_ + camera_offset_);
    if (player_move_animating_) {
        player_move_remaining_ = std::max(0.0F, player_move_remaining_ - delta);
        if (player_move_remaining_ <= 0.0F) {
            player_move_animating_ = false;
        }
    }
    renderWaterAnimation(delta);
    pollPendingCommand();
    if (!command_pending_ && !player_move_animating_) {
        executeNextPathStep(delta);
    }
    updateWorldAutoTick(delta);
}

void MainScene::bootstrapLocalRuntime() {
    local_runtime_ = std::make_unique<pf::client::LocalClientRuntime>();
    if (!local_runtime_->bootstrap()) {
        status_text_ = "本地 Runtime 启动失败: " + local_runtime_->lastError();
        updateHud();
        return;
    }
    syncPlayerCoord();
    selected_coord_ = player_coord_;
    has_selection_ = true;
    status_text_ = "本地 Runtime 已连接：地图/实体/命令来自后端";
    renderWorld();
}

void MainScene::renderWorld() {
    syncPlayerCoord();
    repaintForCurrentViewport();
    renderTerrain();
    renderWaterAnimation(0.0F);
    renderPath();
    renderEntities();
    renderSelection();
    updateHud();
}

void MainScene::renderTerrain() {
    terrain_layer_->removeAllChildren();
    if (!local_runtime_) return;

    struct VisibleCell {
        int x{0};
        int y{0};
        std::string terrain_key;
    };

    visible_water_cells_.clear();
    std::vector<VisibleCell> visible_cells;
    visible_cells.reserve(local_runtime_->snapshot().cells.size());
    for (const auto& cell : local_runtime_->snapshot().cells) {
        if (!isCoordInRenderWindow({cell.x, cell.y})) continue;
        visible_cells.push_back({cell.x, cell.y, cell.terrain_key});
        if (isWaterTerrain(cell.terrain_key)) {
            visible_water_cells_.push_back({cell.x, cell.y});
        }
    }
    if (visible_cells.empty()) return;

    std::sort(visible_cells.begin(), visible_cells.end(), [](const VisibleCell& left, const VisibleCell& right) {
        if (left.y != right.y) return left.y < right.y;
        return left.x < right.x;
    });

    auto* terrain_batch = DrawNode::create();
    const float half = kTileSize * 0.5F;
    auto draw_run = [&](int start_x, int end_x, int y, const std::string& terrain) {
        const float left = static_cast<float>(start_x) * kTileSize - half;
        const float right = static_cast<float>(end_x) * kTileSize + half;
        const float center_y = static_cast<float>(-y) * kTileSize;
        terrain_batch->drawSolidRect(Vec2(left, center_y - half), Vec2(right, center_y + half), terrainBaseColor(terrain));
    };

    int run_start_x = visible_cells.front().x;
    int run_end_x = visible_cells.front().x;
    int run_y = visible_cells.front().y;
    std::string run_terrain = visible_cells.front().terrain_key;
    for (size_t index = 1; index < visible_cells.size(); ++index) {
        const auto& cell = visible_cells[index];
        const bool extends_run = cell.y == run_y && cell.x == run_end_x + 1 && cell.terrain_key == run_terrain;
        if (extends_run) {
            run_end_x = cell.x;
            continue;
        }
        draw_run(run_start_x, run_end_x, run_y, run_terrain);
        run_start_x = cell.x;
        run_end_x = cell.x;
        run_y = cell.y;
        run_terrain = cell.terrain_key;
    }
    draw_run(run_start_x, run_end_x, run_y, run_terrain);

    const bool draw_details = true;
    if (draw_details) {
        for (const auto& cell : visible_cells) {
            drawTerrainDetails(terrain_batch, worldToLocal({cell.x, cell.y}), cell.terrain_key, cell.x, cell.y);
        }
    }

    terrain_layer_->addChild(terrain_batch, 0);

    const bool draw_grid = true;
    if (draw_grid) {
        auto* grid_batch = DrawNode::create();
        const float min_x = (static_cast<float>(render_min_.x) - 0.5F) * kTileSize;
        const float max_x = (static_cast<float>(render_max_.x) + 0.5F) * kTileSize;
        const float min_y = -(static_cast<float>(render_max_.y) + 0.5F) * kTileSize;
        const float max_y = -(static_cast<float>(render_min_.y) - 0.5F) * kTileSize;
        const auto color = gridColor();
        for (int x = render_min_.x; x <= render_max_.x + 1; ++x) {
            const float local_x = (static_cast<float>(x) - 0.5F) * kTileSize;
            grid_batch->drawSolidRect(Vec2(local_x - 0.5F, min_y), Vec2(local_x + 0.5F, max_y), color);
        }
        for (int y = render_min_.y; y <= render_max_.y + 1; ++y) {
            const float local_y = -(static_cast<float>(y) - 0.5F) * kTileSize;
            grid_batch->drawSolidRect(Vec2(min_x, local_y - 0.5F), Vec2(max_x, local_y + 0.5F), color);
        }
        terrain_layer_->addChild(grid_batch, 1);
    }
}

void MainScene::renderWaterAnimation(float delta) {
    if (!water_layer_) return;
    water_anim_accumulator_ += delta;
    if (delta > 0.0F && water_anim_accumulator_ < 0.12F) return;
    water_anim_accumulator_ = 0.0F;
    water_anim_phase_ = (water_anim_phase_ + 1) % 6;

    water_layer_->removeAllChildren();
    if (visible_water_cells_.empty()) return;

    auto* wave_batch = DrawNode::create();
    const auto soft_wave = Color(147 / 255.0F, 197 / 255.0F, 253 / 255.0F, 0.26F);
    const auto soft_highlight = Color(224 / 255.0F, 242 / 255.0F, 254 / 255.0F, 0.22F);

    for (const auto& coord : visible_water_cells_) {
        const auto center = worldToLocal(coord);
        const int seed = terrainNoise(coord.x, coord.y, 900);
        const float drift = static_cast<float>((water_anim_phase_ + seed) % 6) * 1.6F;

        drawRectAt(wave_batch, center, 6.0F + std::fmod(drift + static_cast<float>(seed % 7), 16.0F), 13.0F + static_cast<float>((seed / 5) % 5), 18.0F, 2.0F, soft_wave);
        drawRectAt(wave_batch, center, 20.0F - std::fmod(drift * 0.5F, 7.0F), 32.0F + static_cast<float>((seed / 11) % 4), 16.0F, 2.0F, soft_highlight);
    }
    water_layer_->addChild(wave_batch, 0);
}


void MainScene::renderPath() {
    path_layer_->removeAllChildren();
    if (active_path_.empty()) return;

    auto* path_batch = DrawNode::create();
    const float width = 5.0F;
    const auto link_color = Color(56 / 255.0F, 189 / 255.0F, 248 / 255.0F, 0.34F);
    const auto dot_outer = Color(56 / 255.0F, 189 / 255.0F, 248 / 255.0F, 0.34F);
    const auto dot_inner = Color(250 / 255.0F, 204 / 255.0F, 21 / 255.0F, 0.95F);
    for (size_t index = 0; index < active_path_.size(); ++index) {
        const auto coord = active_path_[index];
        if (isCoordInRenderWindow(coord)) {
            const auto center = worldToLocal(coord);
            path_batch->drawSolidRect(Vec2(center.x - 5.0F, center.y - 5.0F), Vec2(center.x + 5.0F, center.y + 5.0F), dot_outer);
            path_batch->drawSolidRect(Vec2(center.x - 3.0F, center.y - 3.0F), Vec2(center.x + 3.0F, center.y + 3.0F), dot_inner);
        }
        if (index == 0) continue;
        const auto previous = active_path_[index - 1];
        if (!isCoordInRenderWindow(coord) && !isCoordInRenderWindow(previous)) continue;
        const auto from = worldToLocal(previous);
        const auto to = worldToLocal(coord);
        if (std::abs(from.x - to.x) < 0.5F) {
            path_batch->drawSolidRect(Vec2(from.x - width * 0.5F, std::min(from.y, to.y) + width), Vec2(from.x + width * 0.5F, std::max(from.y, to.y) - width), link_color);
        } else {
            path_batch->drawSolidRect(Vec2(std::min(from.x, to.x) + width, from.y - width * 0.5F), Vec2(std::max(from.x, to.x) - width, from.y + width * 0.5F), link_color);
        }
    }
    path_layer_->addChild(path_batch, 4);
}

void MainScene::renderEntities() {
    if (!local_runtime_) return;

    std::unordered_set<std::string> seen;
    for (const auto& entity : local_runtime_->snapshot().entities) {
        const auto render_key = entityRenderKey(entity);
        const auto visual_key = entityVisualKey(entity);
        seen.insert(render_key);

        auto it = entity_nodes_.find(render_key);
        bool created = false;
        if (it == entity_nodes_.end() || it->second.visual_key != visual_key || it->second.node == nullptr) {
            if (it != entity_nodes_.end() && it->second.node != nullptr) {
                it->second.node->removeFromParent();
                entity_nodes_.erase(it);
            }
            EntityRenderState state;
            state.node = createEntityNode(entity);
            state.visual_key = visual_key;
            state.coord = {entity.x, entity.y};
            state.has_coord = true;
            addAt(entity_layer_, state.node, worldToLocal({entity.x, entity.y}), 10);

            if (entity.max_health > 0) {
                auto* hp = DrawNode::create();
                const float width = 28.0F;
                const float height = 4.0F;
                const float ratio = std::clamp(static_cast<float>(entity.health) / static_cast<float>(std::max(1, entity.max_health)), 0.0F, 1.0F);
                const auto fill = ratio > 0.45F
                    ? Color(34 / 255.0F, 197 / 255.0F, 94 / 255.0F, 0.95F)
                    : Color(248 / 255.0F, 113 / 255.0F, 113 / 255.0F, 0.95F);
                hp->drawSolidRect(Vec2(-width * 0.5F, kTileSize * 0.46F), Vec2(width * 0.5F, kTileSize * 0.46F + height), Color(30 / 255.0F, 41 / 255.0F, 59 / 255.0F, 0.9F));
                hp->drawSolidRect(Vec2(-width * 0.5F, kTileSize * 0.46F), Vec2(-width * 0.5F + width * ratio, kTileSize * 0.46F + height), fill);
                state.node->addChild(hp, 30);
            }

            if (!entity.actor_key.empty() && entity.actor_key != "player") {
                auto status_it = local_runtime_->snapshot().actor_work_status.find(entity.actor_key);
                if (status_it != local_runtime_->snapshot().actor_work_status.end() && !status_it->second.empty()) {
                    auto* label = createUiLabel(11.0F, Vec2(92.0F, 18.0F));
                    label->setAnchorPoint(Vec2(0.5F, 0.5F));
                    label->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
                    label->setTextColor(Color32(248, 250, 252, 255));
                    label->setString(status_it->second);
                    label->setPosition(Vec2(0.0F, kTileSize * 0.62F));
                    state.node->addChild(label, 31);
                }
            }
            it = entity_nodes_.emplace(render_key, state).first;
            created = true;
        }

        const Coord target_coord{entity.x, entity.y};
        const bool visible = isCoordInRenderWindow(target_coord);
        it->second.node->setVisible(visible);
        if (visible) {
            const auto target = worldToLocal(target_coord);
            const bool moved = it->second.has_coord && (it->second.coord.x != target_coord.x || it->second.coord.y != target_coord.y);
            const bool smooth_player = entity.entity_key == "player" && moved && !created;
            if (smooth_player) {
                it->second.node->stopActionByTag(kEntityMoveActionTag);
                auto* move = MoveTo::create(kPlayerMoveTweenSeconds, target);
                move->setTag(kEntityMoveActionTag);
                it->second.node->runAction(move);
                player_move_animating_ = true;
                player_move_remaining_ = kPlayerMoveTweenSeconds;
            } else {
                it->second.node->setPosition(target);
            }
        }
        it->second.coord = target_coord;
        it->second.has_coord = true;
    }

    for (auto it = entity_nodes_.begin(); it != entity_nodes_.end();) {
        if (seen.find(it->first) != seen.end()) {
            ++it;
            continue;
        }
        if (it->second.node != nullptr) {
            it->second.node->removeFromParent();
        }
        it = entity_nodes_.erase(it);
    }
}

void MainScene::renderSelection() {
    overlay_layer_->removeAllChildren();
    if (local_runtime_) {
        auto* interact = DrawNode::create();
        for (const auto& option : local_runtime_->snapshot().options) {
            if (!option.enabled || option.command_kind == pathfinder::world_command::WorldCommandKind::Move) continue;
            for (const auto& entity : local_runtime_->snapshot().entities) {
                if (entity.entity_id.empty() || entity.entity_id != option.target_entity_id) continue;
                const Coord coord{entity.x, entity.y};
                if (!isCoordInRenderWindow(coord)) continue;
                const auto center = worldToLocal(coord);
                drawRectAt(interact, center, 20.0F, 3.0F, 8.0F, 3.0F, interactColor());
                drawRectAt(interact, center, 22.0F, 6.0F, 4.0F, 4.0F, interactColor());
            }
        }
        overlay_layer_->addChild(interact, 18);
    }
    if (!has_selection_ || !isCoordInRenderWindow(selected_coord_)) return;

    auto* selection = DrawNode::create();
    const auto center = worldToLocal(selected_coord_);
    const auto c = Color(56 / 255.0F, 189 / 255.0F, 248 / 255.0F, 0.22F);
    drawRectAt(selection, center, 5.0F, 5.0F, 38.0F, 3.0F, c);
    drawRectAt(selection, center, 5.0F, 40.0F, 38.0F, 3.0F, c);
    drawRectAt(selection, center, 5.0F, 5.0F, 3.0F, 38.0F, c);
    drawRectAt(selection, center, 40.0F, 5.0F, 3.0F, 38.0F, c);
    overlay_layer_->addChild(selection, 20);
}

void MainScene::renderUi() {
    ui_hit_boxes_.clear();
    ui_blocking_rects_.clear();
    renderInventoryBar();
    renderContextPanel();
    renderInventoryPanel();
}

void MainScene::renderInventoryBar() {
    if (!inventory_layer_) return;
    inventory_layer_->removeAllChildren();

    const auto visible_size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();
    const float total_width = kInventoryHotbarSlots * kInventorySlotSize + (kInventoryHotbarSlots - 1) * kInventorySlotGap;
    const float start_x = origin.x + (visible_size.width - total_width) * 0.5F;
    const float y = origin.y + 14.0F;
    ui_blocking_rects_.push_back(Rect(start_x - 8.0F, y - 8.0F, total_width + 16.0F, kInventorySlotSize + 16.0F));

    const auto empty_fill = Color(15 / 255.0F, 23 / 255.0F, 42 / 255.0F, 0.76F);
    const auto selected_fill = Color(30 / 255.0F, 41 / 255.0F, 59 / 255.0F, 0.88F);
    const auto normal_border = Color(148 / 255.0F, 163 / 255.0F, 184 / 255.0F, 0.42F);
    const auto selected_border = Color(250 / 255.0F, 204 / 255.0F, 21 / 255.0F, 0.92F);

    auto* slots = DrawNode::create();
    for (int slot = 0; slot < kInventoryHotbarSlots; ++slot) {
        const float x = start_x + static_cast<float>(slot) * (kInventorySlotSize + kInventorySlotGap);
        const bool selected = slot == selected_hotbar_slot_;
        const bool more_button = slot == kInventoryHotbarSlots - 1;
        const auto fill = selected || (more_button && inventory_panel_open_) ? selected_fill : empty_fill;
        const auto border = selected || (more_button && inventory_panel_open_) ? selected_border : normal_border;
        slots->drawSolidRect(Vec2(x, y), Vec2(x + kInventorySlotSize, y + kInventorySlotSize), fill);
        slots->drawSolidRect(Vec2(x, y), Vec2(x + kInventorySlotSize, y + 2.0F), border);
        slots->drawSolidRect(Vec2(x, y + kInventorySlotSize - 2.0F), Vec2(x + kInventorySlotSize, y + kInventorySlotSize), border);
        slots->drawSolidRect(Vec2(x, y), Vec2(x + 2.0F, y + kInventorySlotSize), border);
        slots->drawSolidRect(Vec2(x + kInventorySlotSize - 2.0F, y), Vec2(x + kInventorySlotSize, y + kInventorySlotSize), border);

        if (more_button) {
            ui_hit_boxes_.push_back(UiHitBox{Rect(x, y, kInventorySlotSize, kInventorySlotSize), UiHitKind::ToggleInventoryPanel, slot});
        } else {
            ui_hit_boxes_.push_back(UiHitBox{Rect(x, y, kInventorySlotSize, kInventorySlotSize), UiHitKind::HotbarSlot, slot});
        }
    }
    inventory_layer_->addChild(slots, 0);

    const float more_x = start_x + static_cast<float>(kInventoryHotbarSlots - 1) * (kInventorySlotSize + kInventorySlotGap);
    auto* more = DrawNode::create();
    const Vec2 more_center(more_x + kInventorySlotSize * 0.5F, y + kInventorySlotSize * 0.5F);
    const auto dot_color = Color(248 / 255.0F, 250 / 255.0F, 252 / 255.0F, 0.95F);
    more->drawDot(Vec2(more_center.x - 7.0F, more_center.y), 2.3F, dot_color);
    more->drawDot(more_center, 2.3F, dot_color);
    more->drawDot(Vec2(more_center.x + 7.0F, more_center.y), 2.3F, dot_color);
    inventory_layer_->addChild(more, 2);

    if (!local_runtime_) return;

    const auto& snapshot = local_runtime_->snapshot();
    const auto& items = snapshot.inventory_items;
    normalizeHotbarSlots(items);
    for (int slot = 0; slot < kHotbarItemSlots; ++slot) {
        const int item_index = itemIndexForHotbarSlot(slot, items);
        if (item_index < 0 || item_index >= static_cast<int>(items.size())) continue;
        const float x = start_x + static_cast<float>(slot) * (kInventorySlotSize + kInventorySlotGap);
        const Vec2 center(x + kInventorySlotSize * 0.5F, y + kInventorySlotSize * 0.54F);
        const auto& item = items[static_cast<size_t>(item_index)];
        if (auto* icon = createInventoryIcon(item)) {
            icon->setScale(0.58F);
            icon->setPosition(center);
            inventory_layer_->addChild(icon, 1);
        }
        if (item.quantity > 1) {
            auto* qty = createUiLabel(13.0F);
            qty->setAnchorPoint(Vec2(1.0F, 0.0F));
            qty->setTextColor(Color32(248, 250, 252, 255));
            qty->setString(std::to_string(item.quantity));
            qty->setPosition(Vec2(x + kInventorySlotSize - 4.0F, y + 2.0F));
            inventory_layer_->addChild(qty, 2);
        }
        addDurabilityBar(inventory_layer_, item, Rect(x, y, kInventorySlotSize, kInventorySlotSize), 3);
    }
}

void MainScene::renderInventoryPanel() {
    if (!context_panel_layer_ || !inventory_panel_open_) return;
    if (!local_runtime_) return;

    const auto visible_size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();
    const float panel_width = 500.0F;
    const float panel_height = 360.0F;
    const float panel_x = origin.x + (visible_size.width - panel_width) * 0.5F;
    const float panel_y = origin.y + 74.0F;
    ui_blocking_rects_.push_back(Rect(panel_x, panel_y, panel_width, panel_height));

    auto* panel = DrawNode::create();
    const auto bg = Color(15 / 255.0F, 23 / 255.0F, 42 / 255.0F, 0.94F);
    const auto border = Color(148 / 255.0F, 163 / 255.0F, 184 / 255.0F, 0.48F);
    const auto active = Color(56 / 255.0F, 189 / 255.0F, 248 / 255.0F, 0.78F);
    const auto inactive = Color(30 / 255.0F, 41 / 255.0F, 59 / 255.0F, 0.86F);
    panel->drawSolidRect(Vec2(panel_x, panel_y), Vec2(panel_x + panel_width, panel_y + panel_height), bg);
    panel->drawSolidRect(Vec2(panel_x, panel_y), Vec2(panel_x + panel_width, panel_y + 2.0F), border);
    panel->drawSolidRect(Vec2(panel_x, panel_y + panel_height - 2.0F), Vec2(panel_x + panel_width, panel_y + panel_height), border);
    panel->drawSolidRect(Vec2(panel_x, panel_y), Vec2(panel_x + 2.0F, panel_y + panel_height), border);
    panel->drawSolidRect(Vec2(panel_x + panel_width - 2.0F, panel_y), Vec2(panel_x + panel_width, panel_y + panel_height), border);

    const Rect inv_tab(panel_x + 14.0F, panel_y + panel_height - 42.0F, 92.0F, 30.0F);
    const Rect craft_tab(panel_x + 112.0F, panel_y + panel_height - 42.0F, 92.0F, 30.0F);
    const Rect knowledge_tab(panel_x + 210.0F, panel_y + panel_height - 42.0F, 92.0F, 30.0F);
    const Rect close_rect(panel_x + panel_width - 42.0F, panel_y + panel_height - 40.0F, 28.0F, 28.0F);
    panel->drawSolidRect(Vec2(inv_tab.getMinX(), inv_tab.getMinY()), Vec2(inv_tab.getMaxX(), inv_tab.getMaxY()), inventory_panel_tab_ == InventoryPanelTab::Inventory ? active : inactive);
    panel->drawSolidRect(Vec2(craft_tab.getMinX(), craft_tab.getMinY()), Vec2(craft_tab.getMaxX(), craft_tab.getMaxY()), inventory_panel_tab_ == InventoryPanelTab::Crafting ? active : inactive);
    panel->drawSolidRect(Vec2(knowledge_tab.getMinX(), knowledge_tab.getMinY()), Vec2(knowledge_tab.getMaxX(), knowledge_tab.getMaxY()), inventory_panel_tab_ == InventoryPanelTab::Knowledge ? active : inactive);
    panel->drawSolidRect(Vec2(close_rect.getMinX(), close_rect.getMinY()), Vec2(close_rect.getMaxX(), close_rect.getMaxY()), inactive);
    context_panel_layer_->addChild(panel, 30);

    ui_hit_boxes_.push_back(UiHitBox{inv_tab, UiHitKind::InventoryTab, -1});
    ui_hit_boxes_.push_back(UiHitBox{craft_tab, UiHitKind::CraftingTab, -1});
    ui_hit_boxes_.push_back(UiHitBox{knowledge_tab, UiHitKind::KnowledgeTab, -1});
    ui_hit_boxes_.push_back(UiHitBox{close_rect, UiHitKind::CloseInventoryPanel, -1});

    auto add_label = [&](const std::string& text, float x, float y, float size, const Color32& color) {
        auto* label = createUiLabel(size);
        label->setAnchorPoint(Vec2(0.5F, 0.5F));
        label->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
        label->setTextColor(color);
        label->setString(text);
        label->setPosition(Vec2(x, y));
        context_panel_layer_->addChild(label, 31);
    };
    add_label("背包", inv_tab.getMidX(), inv_tab.getMidY(), 14.0F, Color32(248, 250, 252, 255));
    add_label("合成", craft_tab.getMidX(), craft_tab.getMidY(), 14.0F, Color32(248, 250, 252, 255));
    add_label("知识", knowledge_tab.getMidX(), knowledge_tab.getMidY(), 14.0F, Color32(248, 250, 252, 255));
    add_label("×", close_rect.getMidX(), close_rect.getMidY(), 18.0F, Color32(248, 250, 252, 255));

    const auto& snapshot = local_runtime_->snapshot();
    if (inventory_panel_tab_ == InventoryPanelTab::Crafting) {
        const auto indexes = pf::client_ui::craftOptionIndexes(snapshot);
        auto* title = createUiLabel(14.0F, Vec2(panel_width - 40.0F, 24.0F));
        title->setAnchorPoint(Vec2(0.0F, 0.5F));
        title->setTextColor(Color32(203, 213, 225, 255));
        title->setString("合成命令来自后端 available_commands，前端只负责提交。当前可合成：");
        title->setPosition(Vec2(panel_x + 20.0F, panel_y + panel_height - 66.0F));
        context_panel_layer_->addChild(title, 31);

        if (indexes.empty()) {
            auto* hint = createUiLabel(16.0F, Vec2(panel_width - 56.0F, 96.0F));
            hint->setAnchorPoint(Vec2(0.5F, 0.5F));
            hint->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
            hint->setTextColor(Color32(203, 213, 225, 255));
            hint->setString("还没有可用合成。\n拾取碎石和木头后，后端会自动给出“制作斧头”命令。");
            hint->setPosition(Vec2(panel_x + panel_width * 0.5F, panel_y + panel_height * 0.5F));
            context_panel_layer_->addChild(hint, 31);
        } else {
            auto* rows = DrawNode::create();
            const float row_x = panel_x + 20.0F;
            const float row_w = panel_width - 40.0F;
            const float row_h = 42.0F;
            const float row_gap = 8.0F;
            const float first_y = panel_y + panel_height - 118.0F;
            const int max_rows = 5;
            for (int row = 0; row < std::min<int>(max_rows, static_cast<int>(indexes.size())); ++row) {
                const size_t option_index = indexes[static_cast<size_t>(row)];
                const auto& option = snapshot.options[option_index];
                const float y = first_y - static_cast<float>(row) * (row_h + row_gap);
                const Rect rect(row_x, y - row_h, row_w, row_h);
                rows->drawSolidRect(Vec2(rect.getMinX(), rect.getMinY()), Vec2(rect.getMaxX(), rect.getMaxY()), Color(22 / 255.0F, 101 / 255.0F, 52 / 255.0F, 0.76F));
                rows->drawSolidRect(Vec2(rect.getMinX(), rect.getMinY()), Vec2(rect.getMaxX(), rect.getMinY() + 2.0F), Color(134 / 255.0F, 239 / 255.0F, 172 / 255.0F, 0.78F));
                rows->drawSolidRect(Vec2(rect.getMinX(), rect.getMaxY() - 2.0F), Vec2(rect.getMaxX(), rect.getMaxY()), Color(134 / 255.0F, 239 / 255.0F, 172 / 255.0F, 0.78F));
                rows->drawSolidRect(Vec2(rect.getMinX(), rect.getMinY()), Vec2(rect.getMinX() + 2.0F, rect.getMaxY()), Color(134 / 255.0F, 239 / 255.0F, 172 / 255.0F, 0.78F));
                rows->drawSolidRect(Vec2(rect.getMaxX() - 2.0F, rect.getMinY()), Vec2(rect.getMaxX(), rect.getMaxY()), Color(134 / 255.0F, 239 / 255.0F, 172 / 255.0F, 0.78F));
                auto* label = createUiLabel(15.0F, Vec2(row_w - 20.0F, row_h - 4.0F));
                label->setAnchorPoint(Vec2(0.5F, 0.5F));
                label->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
                label->setTextColor(Color32(240, 253, 244, 255));
                label->setString(option.label_text.empty() ? commandKindLabel(option.command_kind) : option.label_text);
                label->setPosition(Vec2(rect.getMidX(), rect.getMidY()));
                context_panel_layer_->addChild(label, 32);
                ui_hit_boxes_.push_back(UiHitBox{rect, UiHitKind::CraftOption, static_cast<int>(option_index)});
            }
            context_panel_layer_->addChild(rows, 31);
        }
        return;
    }

    if (inventory_panel_tab_ == InventoryPanelTab::Knowledge) {
        const auto& knowledge = snapshot.knowledge_items;
        if (knowledge.empty()) {
            auto* hint = createUiLabel(16.0F, Vec2(panel_width - 56.0F, 96.0F));
            hint->setAnchorPoint(Vec2(0.5F, 0.5F));
            hint->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
            hint->setTextColor(Color32(203, 213, 225, 255));
            hint->setString("还没有形成知识。\n执行吃、采集、合成等后端命令后，经验会进入知识学习。 ");
            hint->setPosition(Vec2(panel_x + panel_width * 0.5F, panel_y + panel_height * 0.5F));
            context_panel_layer_->addChild(hint, 31);
        } else {
            auto* title = createUiLabel(14.0F, Vec2(panel_width - 40.0F, 24.0F));
            title->setAnchorPoint(Vec2(0.0F, 0.5F));
            title->setTextColor(Color32(203, 213, 225, 255));
            title->setString("你已经记住的知识：");
            title->setPosition(Vec2(panel_x + 20.0F, panel_y + panel_height - 66.0F));
            context_panel_layer_->addChild(title, 31);

            const float row_x = panel_x + 20.0F;
            const float row_w = panel_width - 40.0F;
            const float row_h = 32.0F;
            const float first_y = panel_y + panel_height - 108.0F;
            const int max_rows = 7;
            auto* rows = DrawNode::create();
            for (int row = 0; row < std::min<int>(max_rows, static_cast<int>(knowledge.size())); ++row) {
                const auto& item = knowledge[static_cast<size_t>(row)];
                const float y = first_y - static_cast<float>(row) * (row_h + 7.0F);
                const Rect rect(row_x, y - row_h, row_w, row_h);
                rows->drawSolidRect(Vec2(rect.getMinX(), rect.getMinY()), Vec2(rect.getMaxX(), rect.getMaxY()), Color(30 / 255.0F, 41 / 255.0F, 59 / 255.0F, 0.74F));
                auto* label = createUiLabel(13.0F, Vec2(row_w - 16.0F, row_h - 4.0F));
                label->setAnchorPoint(Vec2(0.0F, 0.5F));
                label->setAlignment(TextHAlignment::LEFT, TextVAlignment::CENTER);
                label->setTextColor(Color32(226, 232, 240, 255));
                label->setString(pf::client_ui::formatKnowledgeLine(item));
                label->setPosition(Vec2(rect.getMinX() + 8.0F, rect.getMidY()));
                context_panel_layer_->addChild(label, 32);
            }
            context_panel_layer_->addChild(rows, 31);
        }
        return;
    }

    const auto& items = snapshot.inventory_items;
    normalizeHotbarSlots(items);
    const int columns = 8;
    const int visible_rows = 4;
    const float cell = 46.0F;
    const float gap = 8.0F;
    const float grid_x = panel_x + 20.0F;
    const float grid_y = panel_y + panel_height - 98.0F;
    const int capacity_slots = std::max(kHotbarItemSlots, snapshot.inventory_capacity_slots);
    const int display_slots = std::max(capacity_slots, static_cast<int>(items.size()));
    const int total_rows = std::max(1, (display_slots + columns - 1) / columns);
    const int max_scroll_row = std::max(0, total_rows - visible_rows);
    inventory_scroll_row_ = std::clamp(inventory_scroll_row_, 0, max_scroll_row);
    const int start_slot = inventory_scroll_row_ * columns;
    const int visible_slots = columns * visible_rows;

    auto* capacity = createUiLabel(13.0F, Vec2(116.0F, 24.0F));
    capacity->setAnchorPoint(Vec2(1.0F, 0.5F));
    capacity->setTextColor(Color32(203, 213, 225, 255));
    capacity->setString("容量 " + std::to_string(items.size()) + " / " + std::to_string(capacity_slots));
    capacity->setPosition(Vec2(panel_x + panel_width - 18.0F, panel_y + panel_height - 58.0F));
    context_panel_layer_->addChild(capacity, 31);

    if (selected_inventory_index_ >= 0 && selected_inventory_index_ < static_cast<int>(items.size())) {
        const auto& selected_item = items[static_cast<size_t>(selected_inventory_index_)];
        auto* selected_label = createUiLabel(13.0F, Vec2(190.0F, 24.0F));
        selected_label->setAnchorPoint(Vec2(0.0F, 0.5F));
        selected_label->setTextColor(Color32(250, 204, 21, 255));
        const auto name = selected_item.display_name.empty() ? selected_item.entity_key : selected_item.display_name;
        selected_label->setString(name + " ×" + std::to_string(selected_item.quantity));
        selected_label->setPosition(Vec2(panel_x + 220.0F, panel_y + panel_height - 58.0F));
        context_panel_layer_->addChild(selected_label, 31);
    }

    auto* grid = DrawNode::create();
    const auto empty_fill = Color(15 / 255.0F, 23 / 255.0F, 42 / 255.0F, 0.42F);
    const auto slot_fill = Color(30 / 255.0F, 41 / 255.0F, 59 / 255.0F, 0.82F);
    const auto overflow_fill = Color(69 / 255.0F, 26 / 255.0F, 3 / 255.0F, 0.82F);
    const auto grid_line = Color(148 / 255.0F, 163 / 255.0F, 184 / 255.0F, 0.58F);
    const auto overflow_line = Color(251 / 255.0F, 146 / 255.0F, 60 / 255.0F, 0.78F);
    for (int visible_index = 0; visible_index < visible_slots; ++visible_index) {
        const int slot_index = start_slot + visible_index;
        if (slot_index >= display_slots) continue;
        const int col = visible_index % columns;
        const int row = visible_index / columns;
        const float x = grid_x + static_cast<float>(col) * (cell + gap);
        const float y = grid_y - static_cast<float>(row) * (cell + gap);
        const Rect rect(x, y - cell, cell, cell);
        const bool has_item = slot_index < static_cast<int>(items.size());
        const bool overflow = slot_index >= capacity_slots;
        const bool selected = selected_inventory_index_ == slot_index;
        const auto fill = selected ? Color(30 / 255.0F, 64 / 255.0F, 175 / 255.0F, 0.85F) : (overflow ? overflow_fill : (has_item ? slot_fill : empty_fill));
        const auto line = selected ? Color(250 / 255.0F, 204 / 255.0F, 21 / 255.0F, 0.9F) : (overflow ? overflow_line : grid_line);
        grid->drawSolidRect(Vec2(rect.getMinX(), rect.getMinY()), Vec2(rect.getMaxX(), rect.getMaxY()), fill);
        grid->drawSolidRect(Vec2(rect.getMinX(), rect.getMinY()), Vec2(rect.getMaxX(), rect.getMinY() + 2.0F), line);
        grid->drawSolidRect(Vec2(rect.getMinX(), rect.getMaxY() - 2.0F), Vec2(rect.getMaxX(), rect.getMaxY()), line);
        grid->drawSolidRect(Vec2(rect.getMinX(), rect.getMinY()), Vec2(rect.getMinX() + 2.0F, rect.getMaxY()), line);
        grid->drawSolidRect(Vec2(rect.getMaxX() - 2.0F, rect.getMinY()), Vec2(rect.getMaxX(), rect.getMaxY()), line);
        if (has_item) {
            ui_hit_boxes_.push_back(UiHitBox{rect, UiHitKind::InventoryItem, slot_index});
        }
    }
    context_panel_layer_->addChild(grid, 31);

    for (int visible_index = 0; visible_index < visible_slots; ++visible_index) {
        const int slot_index = start_slot + visible_index;
        if (slot_index < 0 || slot_index >= static_cast<int>(items.size())) continue;
        const int col = visible_index % columns;
        const int row = visible_index / columns;
        const float x = grid_x + static_cast<float>(col) * (cell + gap);
        const float y = grid_y - static_cast<float>(row) * (cell + gap);
        const auto& item = items[static_cast<size_t>(slot_index)];
        if (auto* icon = createInventoryIcon(item)) {
            icon->setScale(0.58F);
            icon->setPosition(Vec2(x + cell * 0.5F, y - cell * 0.48F));
            context_panel_layer_->addChild(icon, 32);
        }
        if (item.quantity > 1) {
            auto* qty = createUiLabel(12.0F);
            qty->setAnchorPoint(Vec2(1.0F, 0.0F));
            qty->setTextColor(Color32(248, 250, 252, 255));
            qty->setString(std::to_string(item.quantity));
            qty->setPosition(Vec2(x + cell - 4.0F, y - cell + 2.0F));
            context_panel_layer_->addChild(qty, 33);
        }
        addDurabilityBar(context_panel_layer_, item, Rect(x, y - cell, cell, cell), 34);
    }

    const float track_x = panel_x + panel_width - 20.0F;
    const float track_top = grid_y;
    const float track_bottom = grid_y - static_cast<float>(visible_rows - 1) * (cell + gap) - cell;
    auto* scrollbar = DrawNode::create();
    scrollbar->drawSolidRect(Vec2(track_x, track_bottom), Vec2(track_x + 4.0F, track_top), Color(51 / 255.0F, 65 / 255.0F, 85 / 255.0F, 0.74F));
    const float track_height = track_top - track_bottom;
    const float thumb_height = max_scroll_row == 0 ? track_height : std::max(28.0F, track_height * static_cast<float>(visible_rows) / static_cast<float>(total_rows));
    const float thumb_range = std::max(0.0F, track_height - thumb_height);
    const float thumb_y = track_top - thumb_height - (max_scroll_row == 0 ? 0.0F : thumb_range * static_cast<float>(inventory_scroll_row_) / static_cast<float>(max_scroll_row));
    scrollbar->drawSolidRect(Vec2(track_x - 1.0F, thumb_y), Vec2(track_x + 5.0F, thumb_y + thumb_height), Color(56 / 255.0F, 189 / 255.0F, 248 / 255.0F, 0.9F));
    context_panel_layer_->addChild(scrollbar, 32);

    auto* tip = createUiLabel(13.0F, Vec2(panel_width - 32.0F, 24.0F));
    tip->setAnchorPoint(Vec2(0.0F, 0.5F));
    tip->setTextColor(Color32(148, 163, 184, 255));
    tip->setString("滚轮翻背包；点物品再点操作栏可放入；容量扩展由后端物品/命令提供。");
    tip->setPosition(Vec2(panel_x + 18.0F, panel_y + 22.0F));
    context_panel_layer_->addChild(tip, 31);
}

void MainScene::renderContextPanel() {
    if (!context_panel_layer_) return;
    context_panel_layer_->removeAllChildren();
    action_hit_boxes_.clear();
    if (!local_runtime_ || !has_selection_) return;

    const auto entity = entityAt(selected_coord_);
    if (!entity) return;

    const auto visible_size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();
    const bool adjacent = isAdjacentToPlayer(selected_coord_);
    const auto actions = actionsForEntity(entity->entity_id);
    const bool is_npc = !entity->actor_key.empty() && entity->actor_key != "player";
    const auto& snapshot = local_runtime_->snapshot();
    std::vector<pf::client::LocalInventoryItemView> npc_items;
    if (is_npc) {
        auto inv_it = snapshot.actor_inventory_items.find(entity->actor_key);
        if (inv_it != snapshot.actor_inventory_items.end()) npc_items = inv_it->second;
    }
    std::vector<pf::client::LocalKnowledgeView> npc_knowledge;
    if (is_npc) {
        for (const auto& item : snapshot.knowledge_items) {
            if (item.actor_key == entity->actor_key) npc_knowledge.push_back(item);
        }
    }

    const float panel_width = is_npc ? 340.0F : 260.0F;
    const float row_height = 34.0F;
    const float header_height = 54.0F;
    const int action_rows = std::max(1, static_cast<int>(actions.size()));
    const int npc_rows = is_npc ? (2 + std::max(1, std::min<int>(3, npc_items.size())) + std::max(1, std::min<int>(4, npc_knowledge.size()))) : 0;
    const int rows = action_rows + npc_rows;
    const float panel_height = header_height + static_cast<float>(rows) * row_height + 14.0F;
    const auto target_screen = world_origin_ + camera_offset_ + worldToLocal(selected_coord_) * zoom_;
    float panel_x = target_screen.x + 28.0F;
    float panel_y = target_screen.y - panel_height * 0.5F;
    panel_x = std::clamp(panel_x, origin.x + 12.0F, origin.x + visible_size.width - panel_width - 12.0F);
    panel_y = std::clamp(panel_y, origin.y + 66.0F, origin.y + visible_size.height - panel_height - 12.0F);
    ui_blocking_rects_.push_back(Rect(panel_x, panel_y, panel_width, panel_height));

    auto* panel = DrawNode::create();
    const auto bg = Color(15 / 255.0F, 23 / 255.0F, 42 / 255.0F, 0.90F);
    const auto line = Color(56 / 255.0F, 189 / 255.0F, 248 / 255.0F, 0.66F);
    panel->drawSolidRect(Vec2(panel_x, panel_y), Vec2(panel_x + panel_width, panel_y + panel_height), bg);
    panel->drawSolidRect(Vec2(panel_x, panel_y + panel_height - 2.0F), Vec2(panel_x + panel_width, panel_y + panel_height), line);
    panel->drawSolidRect(Vec2(panel_x, panel_y), Vec2(panel_x + panel_width, panel_y + 2.0F), line);
    panel->drawSolidRect(Vec2(panel_x, panel_y), Vec2(panel_x + 2.0F, panel_y + panel_height), line);
    panel->drawSolidRect(Vec2(panel_x + panel_width - 2.0F, panel_y), Vec2(panel_x + panel_width, panel_y + panel_height), line);
    context_panel_layer_->addChild(panel, 0);

    auto* title = createUiLabel(16.0F, Vec2(panel_width - 24.0F, 24.0F));
    title->setAnchorPoint(Vec2(0.0F, 1.0F));
    title->setTextColor(Color32(248, 250, 252, 255));
    title->setString(entity->display_name_key.empty() ? entity->entity_key : entity->display_name_key);
    title->setPosition(Vec2(panel_x + 12.0F, panel_y + panel_height - 10.0F));
    context_panel_layer_->addChild(title, 1);

    auto* subtitle = createUiLabel(12.0F, Vec2(panel_width - 24.0F, 20.0F));
    subtitle->setAnchorPoint(Vec2(0.0F, 1.0F));
    subtitle->setTextColor(Color32(148, 163, 184, 255));
    subtitle->setString(actions.empty() && !adjacent ? "靠近后会出现可用操作" : "选择一个后端命令");
    subtitle->setPosition(Vec2(panel_x + 12.0F, panel_y + panel_height - 32.0F));
    context_panel_layer_->addChild(subtitle, 1);

    if (actions.empty()) {
        auto* empty = createUiLabel(14.0F, Vec2(panel_width - 24.0F, row_height));
        empty->setAnchorPoint(Vec2(0.5F, 0.5F));
        empty->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
        empty->setTextColor(Color32(203, 213, 225, 255));
        empty->setString(adjacent ? "暂无可用操作" : "先靠近目标再操作");
        empty->setPosition(Vec2(panel_x + panel_width * 0.5F, panel_y + panel_height - header_height - row_height * 0.5F));
        context_panel_layer_->addChild(empty, 1);
        if (!is_npc) return;
    }

    for (size_t index = 0; index < actions.size(); ++index) {
        const float row_x = panel_x + 10.0F;
        const float row_y = panel_y + panel_height - header_height - static_cast<float>(index + 1) * row_height;
        const float row_w = panel_width - 20.0F;
        const auto row_fill = Color(30 / 255.0F, 41 / 255.0F, 59 / 255.0F, 0.82F);
        panel->drawSolidRect(Vec2(row_x, row_y + 3.0F), Vec2(row_x + row_w, row_y + row_height - 3.0F), row_fill);

        auto* label = createUiLabel(14.0F, Vec2(row_w - 18.0F, row_height - 6.0F));
        label->setAnchorPoint(Vec2(0.5F, 0.5F));
        label->setAlignment(TextHAlignment::CENTER, TextVAlignment::CENTER);
        label->setTextColor(Color32(226, 232, 240, 255));
        label->setString(actions[index].label_text.empty() ? commandKindLabel(actions[index].command_kind) : actions[index].label_text);
        label->setPosition(Vec2(row_x + row_w * 0.5F, row_y + row_height * 0.5F));
        context_panel_layer_->addChild(label, 1);

        action_hit_boxes_.push_back(ActionHitBox{Rect(row_x, row_y + 3.0F, row_w, row_height - 6.0F), actions[index]});
    }

    if (is_npc) {
        float cursor_y = panel_y + panel_height - header_height - static_cast<float>(action_rows) * row_height - 8.0F;
        auto add_section_title = [&](const std::string& text) {
            auto* label = createUiLabel(13.0F, Vec2(panel_width - 24.0F, 22.0F));
            label->setAnchorPoint(Vec2(0.0F, 1.0F));
            label->setTextColor(Color32(125, 211, 252, 255));
            label->setString(text);
            label->setPosition(Vec2(panel_x + 12.0F, cursor_y));
            context_panel_layer_->addChild(label, 2);
            cursor_y -= row_height;
        };
        auto add_info_row = [&](const std::string& text) {
            const float row_x = panel_x + 10.0F;
            const float row_w = panel_width - 20.0F;
            panel->drawSolidRect(Vec2(row_x, cursor_y - row_height + 5.0F), Vec2(row_x + row_w, cursor_y - 2.0F), Color(30 / 255.0F, 41 / 255.0F, 59 / 255.0F, 0.58F));
            auto* label = createUiLabel(12.0F, Vec2(row_w - 16.0F, row_height - 6.0F));
            label->setAnchorPoint(Vec2(0.0F, 0.5F));
            label->setAlignment(TextHAlignment::LEFT, TextVAlignment::CENTER);
            label->setTextColor(Color32(226, 232, 240, 255));
            label->setString(text);
            label->setPosition(Vec2(row_x + 8.0F, cursor_y - row_height * 0.5F));
            context_panel_layer_->addChild(label, 2);
            cursor_y -= row_height;
        };

        add_section_title("NPC背包");
        if (npc_items.empty()) {
            add_info_row("空");
        } else {
            for (int i = 0; i < std::min<int>(3, npc_items.size()); ++i) {
                const auto& item = npc_items[static_cast<size_t>(i)];
                add_info_row(item.display_name + " ×" + std::to_string(item.quantity));
            }
        }

        add_section_title("NPC知识");
        if (npc_knowledge.empty()) {
            add_info_row("还没有知识");
        } else {
            for (int i = 0; i < std::min<int>(4, npc_knowledge.size()); ++i) {
                add_info_row(pf::client_ui::formatKnowledgeLine(npc_knowledge[static_cast<size_t>(i)]));
            }
        }
    }
}

void MainScene::updateHud() {
    if (status_label_) {
        std::string text = status_text_;
        if (local_runtime_) {
            for (const auto& entity : local_runtime_->snapshot().entities) {
                if (entity.entity_key == "player" && entity.max_health > 0) {
                    text += "\n生命：" + std::to_string(entity.health) + "/" + std::to_string(entity.max_health);
                    break;
                }
            }
            if (!local_runtime_->snapshot().events.empty()) {
                text += "\n事件：" + local_runtime_->snapshot().events.back();
            }
        }
        status_label_->setString(text);
        status_label_->setVisible(true);
    }
    if (selection_label_) {
        selection_label_->setString("双击地块寻路；点击目标查看操作；世界会自动推进。");
        selection_label_->setVisible(true);
    }
    renderUi();
}

void MainScene::updateSelectionPanel() {
    renderUi();
}

void MainScene::syncPlayerCoord() {
    if (!local_runtime_) return;
    for (const auto& entity : local_runtime_->snapshot().entities) {
        if (entity.entity_key == "player") {
            player_coord_ = {entity.x, entity.y};
            return;
        }
    }
}

void MainScene::startPathTo(Coord target) {
    active_path_ = buildVisualPath(player_coord_, target);
    path_queue_ = active_path_;
    path_step_accumulator_ = kPathStepSeconds;
    status_text_ = "开始向目标移动：" + std::to_string(target.x) + "," + std::to_string(target.y);
}

void MainScene::executeNextPathStep(float delta) {
    if (!local_runtime_ || path_queue_.empty()) return;
    path_step_accumulator_ += delta;
    if (path_step_accumulator_ < kPathStepSeconds) return;
    path_step_accumulator_ = 0.0F;

    const Coord next = path_queue_.front();
    auto option = local_runtime_->findMoveOptionTo(next.x, next.y);
    if (!option) {
        status_text_ = "后端当前不允许移动到：" + std::to_string(next.x) + "," + std::to_string(next.y);
        path_queue_.clear();
        active_path_.clear();
        renderWorld();
        return;
    }

    submitOptionAndRender(*option, true);
}

void MainScene::updateWorldAutoTick(float delta) {
    if (!local_runtime_ || command_pending_ || player_move_animating_) return;
    if (!path_queue_.empty() || inventory_panel_open_ || dragging_ || mouse_down_) {
        world_tick_accumulator_ = 0.0F;
        return;
    }

    world_tick_accumulator_ += delta;
    if (world_tick_accumulator_ < kWorldAutoTickSeconds) return;
    world_tick_accumulator_ = 0.0F;

    for (const auto& option : local_runtime_->snapshot().options) {
        if (!option.enabled || option.command_kind != pathfinder::world_command::WorldCommandKind::Wait) continue;
        pending_auto_tick_ = true;
        submitOptionAndRender(option, false);
        return;
    }
}

void MainScene::pollPendingCommand() {
    if (!command_pending_ || !pending_command_.valid()) return;
    using namespace std::chrono_literals;
    if (pending_command_.wait_for(0ms) != std::future_status::ready) return;
    const bool ok = pending_command_.get();
    command_pending_ = false;
    finishSubmittedOption(ok);
}

void MainScene::submitOptionAndRender(const pf::client::LocalCommandOptionView& option, bool consumes_path_step) {
    if (!local_runtime_ || command_pending_) return;
    pending_option_ = option;
    pending_consumes_path_step_ = consumes_path_step;
    command_pending_ = true;
    if (!pending_auto_tick_) {
        status_text_ = "正在执行: " + (option.label_text.empty() ? option.command_key : option.label_text);
    }
    updateHud();
    pending_command_ = std::async(std::launch::async, [this, option_id = option.option_id]() {
        return local_runtime_ && local_runtime_->submitOption(option_id);
    });
}

void MainScene::finishSubmittedOption(bool ok) {
    if (!ok) {
        status_text_ = "命令失败: " + (local_runtime_ ? local_runtime_->lastError() : std::string("runtime_unavailable"));
        path_queue_.clear();
        active_path_.clear();
        pending_consumes_path_step_ = false;
        pending_auto_tick_ = false;
        renderWorld();
        return;
    }

    if (pending_consumes_path_step_ && !path_queue_.empty()) {
        path_queue_.erase(path_queue_.begin());
        active_path_ = path_queue_;
        path_step_accumulator_ = kPathStepSeconds;
    }
    pending_consumes_path_step_ = false;
    if (pending_auto_tick_) {
        status_text_ = "世界自动推进";
        pending_auto_tick_ = false;
    } else {
        status_text_ = "执行成功: " + (pending_option_.label_text.empty() ? pending_option_.command_key : pending_option_.label_text);
    }
    renderWorld();
}

void MainScene::repaintForCurrentViewport() {
    const auto visible_size = Director::getInstance()->getVisibleSize();
    const auto origin = Director::getInstance()->getVisibleOrigin();
    const Coord a = screenToCoord(Vec2(origin.x, origin.y));
    const Coord b = screenToCoord(Vec2(origin.x + visible_size.width, origin.y + visible_size.height));
    render_min_.x = std::min(a.x, b.x) - kRenderMarginTiles;
    render_max_.x = std::max(a.x, b.x) + kRenderMarginTiles;
    render_min_.y = std::min(a.y, b.y) - kRenderMarginTiles;
    render_max_.y = std::max(a.y, b.y) + kRenderMarginTiles;
}

bool MainScene::isCoordInRenderWindow(Coord coord) const {
    return coord.x >= render_min_.x && coord.x <= render_max_.x &&
           coord.y >= render_min_.y && coord.y <= render_max_.y;
}

bool MainScene::isAdjacentToPlayer(Coord coord) const {
    return std::abs(coord.x - player_coord_.x) <= 1 && std::abs(coord.y - player_coord_.y) <= 1;
}

std::optional<pf::client::LocalEntityView> MainScene::entityAt(Coord coord) const {
    if (!local_runtime_) return std::nullopt;
    const auto matches = [&](const pf::client::LocalEntityView& entity) {
        return entity.x == coord.x && entity.y == coord.y && entity.entity_key != "player";
    };

    for (const auto& entity : local_runtime_->snapshot().entities) {
        if (matches(entity) && !entity.is_resource_node) {
            return entity;
        }
    }
    for (const auto& entity : local_runtime_->snapshot().entities) {
        if (matches(entity)) {
            return entity;
        }
    }
    return std::nullopt;
}

std::vector<pf::client::LocalCommandOptionView> MainScene::actionsForEntity(const std::string& entity_id) const {
    std::vector<pf::client::LocalCommandOptionView> actions;
    if (!local_runtime_ || entity_id.empty()) return actions;
    for (const auto& option : local_runtime_->snapshot().options) {
        if (!option.enabled) continue;
        if (option.command_kind == pathfinder::world_command::WorldCommandKind::Move) continue;
        if (option.target_entity_id == entity_id) actions.push_back(option);
    }
    return actions;
}

bool MainScene::handleActionPanelClick(const Vec2& screen) {
    if (command_pending_) return !action_hit_boxes_.empty();
    for (const auto& hit : action_hit_boxes_) {
        if (!hit.rect.containsPoint(screen)) continue;
        submitOptionAndRender(hit.option);
        active_path_.clear();
        path_queue_.clear();
        renderWorld();
        return true;
    }
    return false;
}

bool MainScene::handleUiClick(const Vec2& screen) {
    if (!local_runtime_) return isUiPointBlocked(screen);
    const auto& items = local_runtime_->snapshot().inventory_items;
    normalizeHotbarSlots(items);

    for (auto it = ui_hit_boxes_.rbegin(); it != ui_hit_boxes_.rend(); ++it) {
        if (!it->rect.containsPoint(screen)) continue;
        switch (it->kind) {
            case UiHitKind::ToggleInventoryPanel:
                inventory_panel_open_ = !inventory_panel_open_;
                selected_hotbar_slot_ = -1;
                selected_inventory_index_ = -1;
                renderWorld();
                return true;
            case UiHitKind::CloseInventoryPanel:
                inventory_panel_open_ = false;
                selected_hotbar_slot_ = -1;
                selected_inventory_index_ = -1;
                renderWorld();
                return true;
            case UiHitKind::InventoryTab:
                inventory_panel_tab_ = InventoryPanelTab::Inventory;
                renderWorld();
                return true;
            case UiHitKind::CraftingTab:
                inventory_panel_tab_ = InventoryPanelTab::Crafting;
                selected_hotbar_slot_ = -1;
                selected_inventory_index_ = -1;
                renderWorld();
                return true;
            case UiHitKind::KnowledgeTab:
                inventory_panel_tab_ = InventoryPanelTab::Knowledge;
                selected_hotbar_slot_ = -1;
                selected_inventory_index_ = -1;
                renderWorld();
                return true;
            case UiHitKind::CraftOption:
                if (it->index >= 0 && it->index < static_cast<int>(local_runtime_->snapshot().options.size())) {
                    submitOptionAndRender(local_runtime_->snapshot().options[static_cast<size_t>(it->index)]);
                }
                return true;
            case UiHitKind::HotbarSlot:
                if (it->index < 0 || it->index >= kHotbarItemSlots) return true;
                if (selected_inventory_index_ >= 0 && selected_inventory_index_ < static_cast<int>(items.size())) {
                    placeInventoryItemInHotbar(it->index, selected_inventory_index_, items);
                    selected_inventory_index_ = -1;
                    selected_hotbar_slot_ = -1;
                } else if (selected_hotbar_slot_ >= 0 && selected_hotbar_slot_ < kHotbarItemSlots && selected_hotbar_slot_ != it->index) {
                    std::swap(hotbar_item_keys_[static_cast<size_t>(selected_hotbar_slot_)], hotbar_item_keys_[static_cast<size_t>(it->index)]);
                    selected_hotbar_slot_ = -1;
                } else {
                    selected_hotbar_slot_ = it->index;
                    selected_inventory_index_ = -1;
                }
                renderWorld();
                return true;
            case UiHitKind::InventoryItem:
                if (it->index < 0 || it->index >= static_cast<int>(items.size())) return true;
                if (selected_hotbar_slot_ >= 0 && selected_hotbar_slot_ < kHotbarItemSlots) {
                    placeInventoryItemInHotbar(selected_hotbar_slot_, it->index, items);
                    selected_hotbar_slot_ = -1;
                    selected_inventory_index_ = -1;
                } else {
                    selected_inventory_index_ = it->index;
                    selected_hotbar_slot_ = -1;
                }
                renderWorld();
                return true;
        }
    }
    return isUiPointBlocked(screen);
}

bool MainScene::isUiPointBlocked(const Vec2& screen) const {
    for (const auto& rect : ui_blocking_rects_) {
        if (rect.containsPoint(screen)) return true;
    }
    return false;
}

void MainScene::normalizeHotbarSlots(const std::vector<pf::client::LocalInventoryItemView>& items) {
    if (hotbar_item_keys_.size() != static_cast<size_t>(kHotbarItemSlots)) {
        hotbar_item_keys_.assign(static_cast<size_t>(kHotbarItemSlots), std::string{});
    }

    std::unordered_set<std::string> valid_keys;
    valid_keys.reserve(items.size());
    for (const auto& item : items) valid_keys.insert(inventoryItemKey(item));

    std::unordered_set<std::string> used;
    for (auto& key : hotbar_item_keys_) {
        if (key.empty()) continue;
        if (valid_keys.find(key) == valid_keys.end() || used.find(key) != used.end()) {
            key.clear();
            continue;
        }
        used.insert(key);
    }

    for (const auto& item : items) {
        const auto key = inventoryItemKey(item);
        if (used.find(key) != used.end()) continue;
        auto empty = std::find(hotbar_item_keys_.begin(), hotbar_item_keys_.end(), std::string{});
        if (empty == hotbar_item_keys_.end()) break;
        *empty = key;
        used.insert(key);
    }
}

int MainScene::itemIndexForHotbarSlot(int slot, const std::vector<pf::client::LocalInventoryItemView>& items) const {
    if (slot < 0 || slot >= static_cast<int>(hotbar_item_keys_.size())) return -1;
    return findInventoryIndexByKey(items, hotbar_item_keys_[static_cast<size_t>(slot)]);
}

void MainScene::placeInventoryItemInHotbar(int slot, int item_index, const std::vector<pf::client::LocalInventoryItemView>& items) {
    if (slot < 0 || slot >= kHotbarItemSlots) return;
    if (item_index < 0 || item_index >= static_cast<int>(items.size())) return;
    normalizeHotbarSlots(items);

    const auto key = inventoryItemKey(items[static_cast<size_t>(item_index)]);
    auto existing = std::find(hotbar_item_keys_.begin(), hotbar_item_keys_.end(), key);
    const auto target = hotbar_item_keys_.begin() + slot;
    if (existing != hotbar_item_keys_.end() && existing != target) {
        std::iter_swap(existing, target);
    } else {
        *target = key;
    }
}


Node* MainScene::createTerrainNode(const pf::client::LocalCellView& cell) const {
    const auto& terrain = cell.terrain_key;
    if (containsToken(terrain, "forest")) return pf::art::createForestTile(kTileSize);
    if (containsToken(terrain, "water")) return pf::art::createWaterTile(kTileSize);
    if (containsToken(terrain, "stone") || containsToken(terrain, "rock") || containsToken(terrain, "mountain")) return pf::art::createRockyTile(kTileSize);
    return pf::art::createGrassTile(kTileSize);
}

Node* MainScene::createEntityNode(const pf::client::LocalEntityView& entity) const {
    const auto key = entity.entity_key + " " + entity.display_name_key;
    if (entity.entity_key == "player") return pf::art::createPlayer(kTileSize);
    if (containsToken(key, "npc") || containsToken(key, "companion")) return pf::art::createCompanion(kTileSize);
    if (containsToken(key, "wolf") || containsToken(key, "beast")) return pf::art::createWolf(kTileSize);
    if (entity.entity_key == "axe") return pf::art::createAxe(kTileSize);
    if (entity.entity_key == "red_berry") return pf::art::createRedBerry(kTileSize);
    if (entity.entity_key == "decayed_red_berry") return pf::art::createDecayedRedBerry(kTileSize);
    if (containsToken(key, "tree")) return pf::art::createTree(kTileSize);
    if (containsToken(key, "berry") || containsToken(key, "bush")) return pf::art::createBerryBush(kTileSize);
    if (containsToken(key, "loose_stone_node")) return pf::art::createStonePile(kTileSize);
    if (containsToken(key, "stone_flake")) return pf::art::createStoneFlake(kTileSize);
    if (containsToken(key, "wood")) return pf::art::createWood(kTileSize);
    if (entity.entity_key == "camp_fire") return pf::art::createCampFire(kTileSize);
    if (containsToken(key, "torch") || containsToken(key, "fire")) return pf::art::createTorch(kTileSize);
    return pf::art::createLooseStone(kTileSize);
}

Node* MainScene::createInventoryIcon(const pf::client::LocalInventoryItemView& item) const {
    pf::client::LocalEntityView entity;
    entity.entity_key = item.entity_key.empty() ? item.stack_key : item.entity_key;
    entity.display_name_key = entity.entity_key;
    return createEntityNode(entity);
}

std::string MainScene::entityRenderKey(const pf::client::LocalEntityView& entity) const {
    if (entity.entity_key == "player") return "actor:player";
    const auto key = entity.entity_key + " " + entity.display_name_key;
    if (containsToken(key, "companion") || containsToken(key, "npc")) {
        return entity.entity_id.empty() ? "actor:" + entity.entity_key : entity.entity_id;
    }
    if (!entity.entity_id.empty()) return entity.entity_id;
    return entity.entity_key + "@" + std::to_string(entity.x) + "," + std::to_string(entity.y);
}

std::string MainScene::entityVisualKey(const pf::client::LocalEntityView& entity) const {
    std::string status;
    if (local_runtime_ && !entity.actor_key.empty()) {
        auto it = local_runtime_->snapshot().actor_work_status.find(entity.actor_key);
        if (it != local_runtime_->snapshot().actor_work_status.end()) status = it->second;
    }
    return entity.entity_key + "|" + entity.display_name_key + "|hp:" +
           std::to_string(entity.health) + "/" + std::to_string(entity.max_health) +
           "|alive:" + (entity.alive ? "1" : "0") + "|work:" + status;
}

Vec2 MainScene::worldToLocal(const Coord& coord) const {
    return Vec2(static_cast<float>(coord.x) * kTileSize, static_cast<float>(-coord.y) * kTileSize);
}

Vec2 MainScene::screenToLocal(const Vec2& screen) const {
    return (screen - world_origin_ - camera_offset_) / zoom_;
}

MainScene::Coord MainScene::screenToCoord(const Vec2& screen) const {
    const auto local = screenToLocal(screen);
    const int x = static_cast<int>(std::floor((local.x + kTileSize * 0.5F) / kTileSize));
    const int y = static_cast<int>(std::floor((-local.y + kTileSize * 0.5F) / kTileSize));
    return {x, y};
}

std::vector<MainScene::Coord> MainScene::buildVisualPath(Coord from, Coord to) const {
    std::vector<Coord> path;
    Coord cursor = from;
    while (cursor.x != to.x) {
        cursor.x += (to.x > cursor.x) ? 1 : -1;
        path.push_back(cursor);
    }
    while (cursor.y != to.y) {
        cursor.y += (to.y > cursor.y) ? 1 : -1;
        path.push_back(cursor);
    }
    return path;
}

bool MainScene::onMouseDown(EventMouse* event) {
    mouse_down_ = true;
    dragging_ = false;
    mouse_down_pos_ = event->getLocation();
    mouse_down_on_ui_ = isUiPointBlocked(mouse_down_pos_);
    return true;
}

bool MainScene::onMouseUp(EventMouse* event) {
    const auto location = event->getLocation();
    mouse_down_ = false;
    if (dragging_) {
        dragging_ = false;
        mouse_down_on_ui_ = false;
        renderWorld();
        return true;
    }

    if (inventory_panel_open_ && handleUiClick(location)) {
        mouse_down_on_ui_ = false;
        return true;
    }

    if (handleActionPanelClick(location)) {
        mouse_down_on_ui_ = false;
        return true;
    }

    if (handleUiClick(location) || mouse_down_on_ui_) {
        mouse_down_on_ui_ = false;
        return true;
    }

    const Coord coord = screenToCoord(location);
    const double now = Director::getInstance()->getTotalFrames() / 60.0;
    const bool same_tile = coord.x == last_click_coord_.x && coord.y == last_click_coord_.y;
    const bool double_click = (now - last_click_seconds_) <= kDoubleClickSeconds && same_tile;
    selected_coord_ = coord;
    has_selection_ = true;

    const bool selected_entity = entityAt(coord).has_value();
    if (double_click && local_runtime_ && !selected_entity) {
        startPathTo(coord);
        renderWorld();
    } else {
        status_text_ = selected_entity ? "选择目标" : "选择格子";
        renderWorld();
    }

    last_click_seconds_ = now;
    last_click_coord_ = coord;
    mouse_down_on_ui_ = false;
    return true;
}

bool MainScene::onMouseMove(EventMouse* event) {
    if (!mouse_down_) return true;
    if (mouse_down_on_ui_) return true;
    const auto delta = event->getDelta();
    if (!dragging_ && (std::abs(event->getLocation().x - mouse_down_pos_.x) > 4.0F || std::abs(event->getLocation().y - mouse_down_pos_.y) > 4.0F)) {
        dragging_ = true;
    }
    if (dragging_) {
        camera_offset_ += delta;
        updateHud();
    }
    return true;
}

bool MainScene::onMouseScroll(EventMouse* event) {
    if (inventory_panel_open_ && local_runtime_) {
        const auto visible_size = Director::getInstance()->getVisibleSize();
        const auto origin = Director::getInstance()->getVisibleOrigin();
        const float panel_width = 500.0F;
        const float panel_height = 360.0F;
        const float panel_x = origin.x + (visible_size.width - panel_width) * 0.5F;
        const float panel_y = origin.y + 74.0F;
        const Rect panel_rect(panel_x, panel_y, panel_width, panel_height);
        if (panel_rect.containsPoint(event->getLocation())) {
            const int columns = 8;
            const int visible_rows = 4;
            const auto& snapshot = local_runtime_->snapshot();
            const int capacity_slots = std::max(kHotbarItemSlots, snapshot.inventory_capacity_slots);
            const int display_slots = std::max(capacity_slots, static_cast<int>(snapshot.inventory_items.size()));
            const int total_rows = std::max(1, (display_slots + columns - 1) / columns);
            const int max_scroll_row = std::max(0, total_rows - visible_rows);
            const int direction = event->getScrollY() > 0.0F ? -1 : 1;
            const int old_row = inventory_scroll_row_;
            inventory_scroll_row_ = std::clamp(inventory_scroll_row_ + direction, 0, max_scroll_row);
            if (old_row != inventory_scroll_row_) renderWorld();
            return true;
        }
    }

    const float old_zoom = zoom_;
    zoom_ = std::clamp(zoom_ + event->getScrollY() * 0.08F, kMinZoom, kMaxZoom);
    if (old_zoom != zoom_) renderWorld();
    return true;
}
