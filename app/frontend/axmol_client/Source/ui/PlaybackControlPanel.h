#pragma once

#include "axmol/axmol.h"

#include <functional>

namespace pf::ui {

class PlaybackControlPanel final : public ax::Node {
public:
    static PlaybackControlPanel* create(std::function<void()> on_toggle, std::function<void()> on_step);
    bool init(std::function<void()> on_toggle, std::function<void()> on_step);
    void setState(bool playing, int tick);

private:
    void render();

    std::function<void()> on_toggle_;
    std::function<void()> on_step_;
    bool playing_{false};
    int tick_{0};
};

} // namespace pf::ui
