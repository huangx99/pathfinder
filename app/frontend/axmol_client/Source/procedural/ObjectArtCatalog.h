#pragma once

#include "axmol/axmol.h"
#include <string>

namespace pf::art {

ax::Node* createWorldObjectIcon(const std::string& key, float size);
ax::Node* createWorldAgentIcon(const std::string& key, float size);

} // namespace pf::art
