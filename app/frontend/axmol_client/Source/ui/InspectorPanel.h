#pragma once

#include "axmol/axmol.h"
#include <string>
#include <vector>

namespace pf::ui {

class InspectorPanel final : public ax::Node {
public:
    static InspectorPanel* create();
    bool init() override;
    void setLines(const std::string& title, const std::vector<std::string>& lines);
};

} // namespace pf::ui
