#pragma once

#include "axmol/axmol.h"
#include "pathfinder/v3_sandbox/v3_sandbox_types.h"

#include <functional>
#include <string>

namespace pf::ui {

class AgentInfoPanel final : public ax::Node {
public:
    static AgentInfoPanel* create();
    bool init() override;
    void setAgent(const pathfinder::v3_sandbox::V3AgentView* agent);

private:
    struct LayoutTarget {
        ax::Node* parent{nullptr};
        float width{0.0F};
    };

    void addSectionTitle(const std::string& text, float& y, LayoutTarget target, const std::function<void()>& on_click = {});
    void addLine(const std::string& text, float& y, LayoutTarget target, float height = 22.0F, float size = 14.0F, const std::function<void()>& on_click = {});
    void addMeter(const std::string& label_text, double value, const ax::Color& fill, float& y, LayoutTarget target);

    bool inventory_expanded_{true};
    bool knowledge_expanded_{true};
    std::string selected_inventory_key_;
    pathfinder::v3_sandbox::V3AgentView current_agent_;
    bool has_current_agent_{false};
};

} // namespace pf::ui
