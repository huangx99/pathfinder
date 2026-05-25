#pragma once

#include "axmol/axmol.h"
#include "pathfinder/v3_sandbox/v3_sandbox_types.h"

#include <string>

namespace pf::ui {

class AgentInfoPanel final : public ax::Node {
public:
    static AgentInfoPanel* create();
    bool init() override;
    void setAgent(const pathfinder::v3_sandbox::V3AgentView* agent);

private:
    void addSectionTitle(const std::string& text, float& y);
    void addLine(const std::string& text, float& y, float height = 22.0F, float size = 14.0F);
    void addMeter(const std::string& label_text, double value, const ax::Color& fill, float& y);
};

} // namespace pf::ui
