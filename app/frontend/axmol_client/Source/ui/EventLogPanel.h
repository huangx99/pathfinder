#pragma once

#include "axmol/axmol.h"
#include <string>
#include <vector>

namespace pf::ui {

class EventLogPanel final : public ax::Node {
public:
    static EventLogPanel* create();
    bool init() override;
    void setEvents(const std::vector<std::string>& events, const std::string& error);
};

} // namespace pf::ui
