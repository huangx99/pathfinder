#pragma once

#include "local/LocalClientRuntime.h"
#include <string>
#include <vector>

namespace pf::client_ui {

std::vector<size_t> craftOptionIndexes(const pf::client::LocalClientSnapshot& snapshot);
std::string formatKnowledgeLine(const pf::client::LocalKnowledgeView& knowledge);
std::string knowledgeStatusLabel(const std::string& status);

} // namespace pf::client_ui
