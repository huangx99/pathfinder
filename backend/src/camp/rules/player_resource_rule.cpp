#include "pathfinder/camp/camp_runtime.h"
#include <algorithm>
#include <memory>

namespace pathfinder::camp {
namespace {

class CampResourceCommandRule final : public CampRule {
public:
    std::string ruleKey() const override { return "camp.resource_command"; }

    void buildOptions(const CampRuntime& runtime, std::vector<CampCommandOptionView>& options) const override {
        const auto& state = runtime.state();
        const auto& content = runtime.content();
        for (const auto& [resource_key, resource] : content.resources) {
            const auto pool_it = state.resources.find(resource_key);
            if (resource.gather_command_key.empty() || pool_it == state.resources.end() || pool_it->second.current <= 0) continue;
            CampCommandOptionView option;
            option.option_id = "camp:resource:" + resource_key + ":" + std::to_string(state.version);
            option.command_key = resource.gather_command_key;
            option.label_text = resource.gather_label;
            option.command_kind = pathfinder::world_command::WorldCommandKind::Gather;
            option.target_entity_id = resource.entity_id;
            options.push_back(std::move(option));
        }
        for (const auto& [recipe_key, recipe] : content.recipes) {
            if (recipe.command_key.empty() || !runtime.hasItems("camp", recipe.inputs)) continue;
            CampCommandOptionView option;
            option.option_id = "camp:recipe:" + recipe_key + ":" + std::to_string(state.version);
            option.command_key = recipe.command_key;
            option.label_text = recipe.label;
            option.command_kind = pathfinder::world_command::WorldCommandKind::Craft;
            option.target_entity_id = recipe.target_entity_id;
            options.push_back(std::move(option));
        }
        if (const auto* threat = runtime.primaryThreatDefinition(); threat && state.threat_level > 0 && !threat->repel_command_key.empty() && runtime.itemCount("camp", threat->counter_item_key) >= threat->counter_item_quantity) {
            CampCommandOptionView option;
            option.option_id = "camp:threat:" + threat->key + ":" + std::to_string(state.version);
            option.command_key = threat->repel_command_key;
            option.label_text = threat->repel_label.empty() ? ("营地驱赶" + threat->name) : threat->repel_label;
            option.command_kind = pathfinder::world_command::WorldCommandKind::Use;
            option.target_entity_id = threat->entity_id;
            options.push_back(std::move(option));
        }
    }

    bool canHandle(const CampCommandOptionView& option) const override {
        return option.command_key.rfind("camp.camp_", 0) == 0;
    }

    bool execute(CampRuntime& runtime, const CampCommandOptionView& option, std::string& error) const override {
        auto& state = runtime.state();
        const auto& content = runtime.content();
        for (const auto& [resource_key, resource] : content.resources) {
            if (option.command_key != resource.gather_command_key) continue;
            auto pool_it = state.resources.find(resource_key);
            if (pool_it == state.resources.end() || pool_it->second.current <= 0) { error = resource.depleted_text; return false; }
            --pool_it->second.current;
            runtime.addItems("camp", resource.output_items);
            runtime.addEvent(resource.completion_event.empty() ? (resource.gather_label + "。") : resource.completion_event, "", "CampGathered");
            runtime.markChanged(resource.gather_label);
            return true;
        }
        for (const auto& [_, recipe] : content.recipes) {
            if (option.command_key != recipe.command_key) continue;
            if (!runtime.consumeItems("camp", recipe.inputs)) { error = recipe.missing_material_text; return false; }
            runtime.addItems("camp", recipe.outputs);
            runtime.addEvent(recipe.completion_event, "", "CampCrafted");
            runtime.markChanged(recipe.work_label.empty() ? recipe.label : recipe.work_label);
            return true;
        }
        if (const auto* threat = runtime.primaryThreatDefinition(); threat && option.command_key == threat->repel_command_key) {
            if (!runtime.consumeItem("camp", threat->counter_item_key, threat->counter_item_quantity)) { error = threat->missing_item_blocked_reason; return false; }
            state.threat_level = std::max(0, state.threat_level - threat->repel_amount);
            runtime.addEvent(threat->success_action_label.empty() ? ("营地处理了" + threat->name + "。") : ("营地" + threat->success_action_label + "。"), "", "ThreatRepelled");
            runtime.markChanged(threat->resolved_text.empty() ? (threat->name + "退开") : threat->resolved_text);
            return true;
        }
        error = "unknown camp resource command";
        return false;
    }
};

} // namespace

std::unique_ptr<CampRule> makePlayerResourceRule() { return std::make_unique<CampResourceCommandRule>(); }

} // namespace pathfinder::camp
