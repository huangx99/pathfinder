#include "scene/V3SandboxScene.h"
#include "ui/AgentInfoPanel.h"
#include "ui/EventLogPanel.h"
#include "ui/PlaybackControlPanel.h"
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

    auto* title = pf::ui::label("认知生态沙盒", 24.0F, ax::Vec2(210.0F, 34.0F));
    title->setTextColor(ax::Color32(250, 204, 21, 255));
    title->setPosition(250.0F, 704.0F);
    addChild(title, 2);

    playback_panel_ = pf::ui::PlaybackControlPanel::create([this]() { togglePlayback(); }, [this]() { stepOnce(); });
    playback_panel_->setPosition(594.0F, 642.0F);
    addChild(playback_panel_, 12);

    tool_panel_ = pf::ui::ToolPalettePanel::create(&client_, [this](int index) { handleToolClicked(index); });
    tool_panel_->setPosition(14.0F, 40.0F);
    addChild(tool_panel_, 10);

    map_layer_ = pf::world::SandboxMapLayer::create([this](int x, int y) { handleCellClicked(x, y); });
    map_layer_->setPosition(kMapX, kMapY);
    addChild(map_layer_, 4);

    agent_info_panel_ = pf::ui::AgentInfoPanel::create();
    agent_info_panel_->setPosition(986.0F, 96.0F);
    addChild(agent_info_panel_, 10);

    event_log_panel_ = pf::ui::EventLogPanel::create();
    event_log_panel_->setPosition(250.0F, 14.0F);
    addChild(event_log_panel_, 10);

    refreshAll();
    return true;
}

void V3SandboxScene::refreshAll() {
    map_layer_->render(client_.snapshot(), selected_x_, selected_y_);
    tool_panel_->refresh();
    playback_panel_->setState(playing_, client_.snapshot().tick);
    event_log_panel_->setEvents(client_.snapshot().events, client_.lastError());
    updateAgentPanel();
}

void V3SandboxScene::handleCellClicked(int x, int y) {
    selected_x_ = x;
    selected_y_ = y;
    if (const auto* agent = client_.agentAtCell(x, y)) {
        selected_agent_id_ = agent->agent_id;
        refreshAll();
        return;
    }
    selected_agent_id_.clear();
    client_.applySelectedToolToCell(x, y);
    refreshAll();
}

void V3SandboxScene::handleToolClicked(int index) {
    if (index < 0) {
        client_.clearToolSelection();
        refreshAll();
        return;
    }
    const auto& tools = client_.tools();
    if (index < 0 || index >= static_cast<int>(tools.size())) return;
    client_.selectTool(index);
    refreshAll();
}

void V3SandboxScene::togglePlayback() {
    if (playing_) {
        stopPlayback();
    } else {
        startPlayback();
    }
    refreshAll();
}

void V3SandboxScene::stepOnce() {
    client_.tick(1);
    refreshAll();
}

void V3SandboxScene::startPlayback() {
    if (playing_) return;
    playing_ = true;
    schedule([this](float dt) { handlePlaybackTick(dt); }, 0.75F, "v3_playback_tick");
}

void V3SandboxScene::stopPlayback() {
    if (!playing_) return;
    playing_ = false;
    unschedule("v3_playback_tick");
}

void V3SandboxScene::handlePlaybackTick(float) {
    client_.tick(1);
    refreshAll();
}

void V3SandboxScene::updateAgentPanel() {
    const pathfinder::v3_sandbox::V3AgentView* agent = nullptr;
    if (!selected_agent_id_.empty()) agent = client_.findAgent(selected_agent_id_);
    if (!agent) selected_agent_id_.clear();
    agent_info_panel_->setAgent(agent);
}

} // namespace pf::client
