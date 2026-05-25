#pragma once

#include "axmol/axmol.h"
#include "runtime/EngineLocalClient.h"

#include <string>

namespace pf::world { class SandboxMapLayer; }
namespace pf::ui { class ToolPalettePanel; class AgentInfoPanel; class EventLogPanel; class PlaybackControlPanel; }

namespace pf::client {

class SandboxScene final : public ax::Scene {
public:
    static ax::Scene* createScene();
    bool init() override;
    CREATE_FUNC(SandboxScene);

private:
    void refreshAll();
    void handleCellClicked(int x, int y);
    void handleToolClicked(int index);
    void togglePlayback();
    void stepOnce();
    void startPlayback();
    void stopPlayback();
    void handlePlaybackTick(float dt);
    void updateAgentPanel();

    EngineLocalClient client_;
    pf::world::SandboxMapLayer* map_layer_{nullptr};
    pf::ui::ToolPalettePanel* tool_panel_{nullptr};
    pf::ui::AgentInfoPanel* agent_info_panel_{nullptr};
    pf::ui::EventLogPanel* event_log_panel_{nullptr};
    pf::ui::PlaybackControlPanel* playback_panel_{nullptr};
    int selected_x_{-1};
    int selected_y_{-1};
    std::string selected_agent_id_;
    bool playing_{false};
};

} // namespace pf::client
