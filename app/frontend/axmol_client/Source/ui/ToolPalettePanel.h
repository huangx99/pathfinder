#pragma once

#include "axmol/axmol.h"
#include "runtime/V3LocalClient.h"
#include <functional>

namespace pf::ui {

class ToolPalettePanel final : public ax::Node {
public:
    static ToolPalettePanel* create(pf::client::V3LocalClient* client,
                                     std::function<void(int)> on_tool_clicked);
    bool init(pf::client::V3LocalClient* client,
               std::function<void(int)> on_tool_clicked);
    void refresh();

private:
    void buildSkeleton();
    void updateContent();
    void onToolClicked(int index);

    pf::client::V3LocalClient* client_{nullptr};
    std::function<void(int)> on_tool_clicked_;

    ax::Node* bg_{nullptr};
    ax::Node* cancel_button_{nullptr};
    std::vector<ax::Node*> tool_slots_;
    std::vector<ax::Label*> tool_labels_;
    int selected_index_{-1};
};

} // namespace pf::ui
