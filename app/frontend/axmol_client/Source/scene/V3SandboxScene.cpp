#include "scene/V3SandboxScene.h"
#include "ui/EventLogPanel.h"
#include "ui/InspectorPanel.h"
#include "ui/ToolPalettePanel.h"
#include "ui/UiStyle.h"
#include "world/SandboxMapLayer.h"

namespace pf::client {
namespace {
constexpr float kMapX = 250.0F;
constexpr float kMapY = 118.0F;
}

ax::Scene* V3SandboxScene::createScene() {
    return V3SandboxScene::create();
}

bool V3SandboxScene::init() {
    if (!Scene::init()) return false;

    auto* bg = ax::DrawNode::create();
    bg->drawSolidRect(ax::Vec2::ZERO, ax::Vec2(1280.0F, 720.0F), pf::ui::color(3, 7, 18));
    addChild(bg, 0);

    auto* title = pf::ui::label("Pathfinder V3 认知生态沙盒", 26.0F, ax::Vec2(900.0F, 36.0F));
    title->setTextColor(ax::Color32(250, 204, 21, 255));
    title->setPosition(250.0F, 704.0F);
    addChild(title, 2);

    auto* subtitle = pf::ui::label("选择左侧工具，点击地图投放；NPC 只会根据附近感知和自身经历行动。", 15.0F, ax::Vec2(820.0F, 24.0F));
    subtitle->setPosition(250.0F, 672.0F);
    addChild(subtitle, 2);

    tool_panel_ = pf::ui::ToolPalettePanel::create(&client_, [this]() {
        if (client_.selectedTool().kind == ToolKind::Tick) client_.tick(1);
        refreshAll();
    });
    tool_panel_->setPosition(14.0F, 40.0F);
    addChild(tool_panel_, 10);

    map_layer_ = pf::world::SandboxMapLayer::create([this](int x, int y) { handleCellClicked(x, y); });
    map_layer_->setPosition(kMapX, kMapY);
    addChild(map_layer_, 4);

    inspector_panel_ = pf::ui::InspectorPanel::create();
    inspector_panel_->setPosition(986.0F, 250.0F);
    addChild(inspector_panel_, 10);

    event_log_panel_ = pf::ui::EventLogPanel::create();
    event_log_panel_->setPosition(250.0F, 14.0F);
    addChild(event_log_panel_, 10);

    refreshAll();
    return true;
}

void V3SandboxScene::refreshAll() {
    map_layer_->render(client_.snapshot(), selected_x_, selected_y_);
    tool_panel_->refresh();
    event_log_panel_->setEvents(client_.snapshot().events, client_.lastError());
    if (selected_x_ >= 0 && selected_y_ >= 0) updateInspectorForCell(selected_x_, selected_y_);
}

void V3SandboxScene::handleCellClicked(int x, int y) {
    selected_x_ = x;
    selected_y_ = y;
    client_.applySelectedToolToCell(x, y);
    refreshAll();
}

void V3SandboxScene::updateInspectorForCell(int x, int y) {
    std::vector<std::string> lines;
    client_.inspectCell(x, y, lines);
    const auto& snapshot = client_.snapshot();
    for (const auto& cell : snapshot.cells) {
        if (cell.x != x || cell.y != y || cell.agent_ids.empty()) continue;
        std::vector<std::string> agent_lines;
        if (client_.inspectAgent(cell.agent_ids.front(), agent_lines)) {
            lines.push_back("---- NPC ----");
            lines.insert(lines.end(), agent_lines.begin(), agent_lines.end());
        }
        break;
    }
    inspector_panel_->setLines("格子 (" + std::to_string(x) + "," + std::to_string(y) + ")", lines);
}

} // namespace pf::client
