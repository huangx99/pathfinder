#include "pathfinder/camp/camp_runtime.h"
#include <algorithm>
#include <memory>

namespace pathfinder::camp {
namespace {

class ScholarEventRule final : public CampRule {
public:
    std::string ruleKey() const override { return "camp.scholar_event"; }

    void buildOptions(const CampRuntime& runtime, std::vector<CampCommandOptionView>& options) const override {
        const auto& state = runtime.state();
        const auto& content = runtime.content();
        if (content.scholar_entity_id.empty() || content.scholar_reward_card_id.empty()) return;
        const auto card_def = content.knowledge_cards.find(content.scholar_reward_card_id);
        if (card_def == content.knowledge_cards.end()) return;
        const bool already_has_card = std::any_of(state.cards.begin(), state.cards.end(), [&](const CampKnowledgeCard& card) {
            return card.card_id == content.scholar_reward_card_id;
        });
        CampCommandOptionView inspect;
        inspect.option_id = "camp:scholar_inspect:" + std::to_string(state.version);
        inspect.command_key = "camp.scholar_inspect";
        inspect.label_text = already_has_card ? "和流浪学者交谈" : ("向流浪学者学习" + card_def->second.name + "卡");
        inspect.command_kind = already_has_card ? pathfinder::world_command::WorldCommandKind::Inspect : pathfinder::world_command::WorldCommandKind::Teach;
        inspect.target_entity_id = content.scholar_entity_id;
        options.push_back(std::move(inspect));
    }

    bool canHandle(const CampCommandOptionView& option) const override {
        return option.command_key == "camp.scholar_inspect";
    }

    bool execute(CampRuntime& runtime, const CampCommandOptionView&, std::string& error) const override {
        auto& state = runtime.state();
        const auto& content = runtime.content();
        const auto card_def = content.knowledge_cards.find(content.scholar_reward_card_id);
        if (card_def == content.knowledge_cards.end()) { error = "学者没有可教学知识卡"; return false; }
        const bool already_has_card = std::any_of(state.cards.begin(), state.cards.end(), [&](const CampKnowledgeCard& card) {
            return card.card_id == content.scholar_reward_card_id;
        });
        if (!already_has_card) {
            const auto& card = card_def->second;
            state.cards.push_back(CampKnowledgeCard{card.card_id, card.knowledge_key, card.name, card.summary, card.category, card.status, "", card.recoverable, card.locked});
            runtime.addEvent("流浪学者给了你一张“" + card.name + "”知识卡。", {}, "ScholarTaught");
            runtime.markChanged("获得学者知识卡");
        } else {
            runtime.addEvent(content.scholar_repeat_event, {}, "ScholarTalked");
            runtime.markChanged("学者交谈");
        }
        return true;
    }
};

} // namespace

std::unique_ptr<CampRule> makeScholarEventRule() { return std::make_unique<ScholarEventRule>(); }

} // namespace pathfinder::camp
