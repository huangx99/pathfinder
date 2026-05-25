#pragma once

#include "axmol/axmol.h"
#include "runtime/V3LocalClient.h"

namespace pf::world { class SandboxMapLayer; }
namespace pf::ui { class ToolPalettePanel; class InspectorPanel; class EventLogPanel; }

namespace pf::client {

class V3SandboxScene final : public ax::Scene {
public:
    static ax::Scene* createScene();
    bool init() override;
    CREATE_FUNC(V3SandboxScene);

private:
    void refreshAll();
    void handleCellClicked(int x, int y);
    void updateInspectorForCell(int x, int y);

    V3LocalClient client_;
    pf::world::SandboxMapLayer* map_layer_{nullptr};
    pf::ui::ToolPalettePanel* tool_panel_{nullptr};
    pf::ui::InspectorPanel* inspector_panel_{nullptr};
    pf::ui::EventLogPanel* event_log_panel_{nullptr};
    int selected_x_{-1};
    int selected_y_{-1};
};

} // namespace pf::client
