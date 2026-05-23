#include "ui/ClientUiFormat.h"

#include "pathfinder/world_command/world_command_types.h"
#include <sstream>

namespace pf::client_ui {

std::vector<size_t> craftOptionIndexes(const pf::client::LocalClientSnapshot& snapshot) {
    std::vector<size_t> indexes;
    for (size_t index = 0; index < snapshot.options.size(); ++index) {
        const auto& option = snapshot.options[index];
        if (option.enabled && option.command_kind == pathfinder::world_command::WorldCommandKind::Craft) {
            indexes.push_back(index);
        }
    }
    return indexes;
}

std::string knowledgeStatusLabel(const std::string& status) {
    if (status == "active" || status == "Active") return "已确认";
    if (status == "candidate" || status == "Candidate") return "待确认";
    if (status == "hypothesis" || status == "Hypothesis") return "假设";
    if (status == "shared" || status == "Shared") return "听说";
    if (status == "rejected" || status == "Rejected") return "已否定";
    return status.empty() ? "未知" : status;
}

std::string formatKnowledgeLine(const pf::client::LocalKnowledgeView& knowledge) {
    std::ostringstream oss;
    if (!knowledge.summary_text.empty()) {
        oss << knowledge.summary_text;
    } else {
        const auto subject = knowledge.subject_name.empty() ? knowledge.subject_id : knowledge.subject_name;
        const auto action = knowledge.action_name.empty() ? knowledge.action_key : knowledge.action_name;
        const auto effect = knowledge.effect_name.empty() ? knowledge.effect_key : knowledge.effect_name;
        oss << subject << "：" << action << " → " << effect;
    }
    if (!knowledge.recipe_text.empty()) {
        oss << "（原料：" << knowledge.recipe_text << "）";
    }
    oss << "（" << knowledgeStatusLabel(knowledge.status) << "）";
    return oss.str();
}

} // namespace pf::client_ui
