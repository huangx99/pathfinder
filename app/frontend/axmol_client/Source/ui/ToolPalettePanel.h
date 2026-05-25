#pragma once

#include "axmol/axmol.h"
#include "runtime/V3LocalClient.h"
#include <functional>

namespace pf::ui {

class ToolPalettePanel final : public ax::Node {
public:
    static ToolPalettePanel* create(pf::client::V3LocalClient* client, std::function<void()> on_tool_changed);
    bool init(pf::client::V3LocalClient* client, std::function<void()> on_tool_changed);
    void refresh();

private:
    pf::client::V3LocalClient* client_{nullptr};
    std::function<void()> on_tool_changed_;
};

} // namespace pf::ui
