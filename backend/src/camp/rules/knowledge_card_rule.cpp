#include "pathfinder/camp/camp_runtime.h"
#include <memory>

namespace pathfinder::camp {
namespace {

class KnowledgeCardRule final : public CampRule {
public:
    std::string ruleKey() const override { return "camp.knowledge_card"; }

    void buildOptions(const CampRuntime& runtime, std::vector<CampCommandOptionView>& options) const override {
        const auto& state = runtime.state();
        for (const auto& [actor_key, actor] : state.actors) {
            CampCommandOptionView option;
            option.option_id = "camp:manage_cards:" + actor_key + ":" + std::to_string(state.version);
            option.command_key = "camp.manage_knowledge_cards";
            option.label_text = "管理" + actor.name + "的知识卡";
            option.command_kind = pathfinder::world_command::WorldCommandKind::Inspect;
            option.target_actor_key = actor_key;
            option.target_entity_id = actor_key;
            options.push_back(std::move(option));
        }
    }

    bool canHandle(const CampCommandOptionView& option) const override {
        return option.command_key.rfind("camp.assign_card.", 0) == 0 || option.command_key.rfind("camp.recover_card.", 0) == 0 || option.command_key == "camp.manage_knowledge_cards";
    }

    bool execute(CampRuntime& runtime, const CampCommandOptionView& option, std::string& error) const override {
        auto& state = runtime.state();
        if (option.command_key == "camp.manage_knowledge_cards") {
            runtime.markChanged("打开知识卡管理");
            return true;
        }
        const bool assign = option.command_key.rfind("camp.assign_card.", 0) == 0;
        const std::string card_id = option.command_key.substr(assign ? 17 : 18);
        for (auto& card : state.cards) {
            if (card.card_id != card_id) continue;
            if (assign) {
                if (!card.assigned_actor_key.empty()) { error = "知识卡已经分配给别人"; return false; }
                card.assigned_actor_key = option.target_actor_key;
                runtime.addEvent("营地将“" + card.name + "”知识卡分配给" + runtime.actorName(option.target_actor_key) + "。", option.target_actor_key, "KnowledgeCardAssigned");
                runtime.markChanged("知识卡已分配");
                return true;
            }
            if (card.assigned_actor_key != option.target_actor_key) { error = "这张卡不在该 NPC 身上"; return false; }
            if (card.locked) { error = "知识卡正在关键节点使用，暂时不能回收"; return false; }
            card.assigned_actor_key.clear();
            auto actor_it = state.actors.find(option.target_actor_key);
            if (actor_it != state.actors.end() && actor_it->second.task.active) {
                actor_it->second.task.returning_home = true;
                actor_it->second.task.blocked_reason = "返回家园：关键知识卡被收回";
            }
            runtime.addEvent("营地收回了“" + card.name + "”知识卡。", option.target_actor_key, "KnowledgeCardRecovered");
            runtime.markChanged("知识卡已收回");
            return true;
        }
        error = "找不到知识卡";
        return false;
    }
};

} // namespace

std::unique_ptr<CampRule> makeKnowledgeCardRule() { return std::make_unique<KnowledgeCardRule>(); }

} // namespace pathfinder::camp
