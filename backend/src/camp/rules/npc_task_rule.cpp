#include "pathfinder/camp/camp_runtime.h"
#include <algorithm>
#include <memory>

namespace pathfinder::camp {
namespace {

pathfinder::world_command::WorldCommandKind taskKind(const CampTaskDefinition& task) {
    if (!task.resource_key.empty()) return pathfinder::world_command::WorldCommandKind::Gather;
    if (!task.recipe_key.empty()) return pathfinder::world_command::WorldCommandKind::Craft;
    return pathfinder::world_command::WorldCommandKind::Use;
}

CampNpcTask makeTask(const CampTaskDefinition& definition) {
    CampNpcTask task;
    task.active = true;
    task.task_key = definition.key;
    task.label = definition.label;
    task.duration = std::max(1, definition.duration);
    task.outside_safe_zone = definition.outside_safe_zone;
    task.required_card_key = definition.required_card_key;
    task.resource_key = definition.resource_key;
    task.recipe_key = definition.recipe_key;
    task.threat_key = definition.threat_key;
    task.nodes = definition.nodes;
    if (task.nodes.empty()) {
        task.nodes = task.outside_safe_zone
            ? std::vector<std::string>{"接令", "检查知识", "前往圈外", "执行", "返回交付"}
            : std::vector<std::string>{"接令", "检查知识", "准备材料", "执行", "交付"};
    }
    task.node_index = 0;
    return task;
}

class NpcTaskRule final : public CampRule {
public:
    std::string ruleKey() const override { return "camp.npc_task"; }

    void buildOptions(const CampRuntime& runtime, std::vector<CampCommandOptionView>& options) const override {
        const auto& state = runtime.state();
        const auto& content = runtime.content();
        for (const auto& [actor_key, actor] : state.actors) {
            if (actor.task.active) continue;
            for (const auto& [_, task] : content.tasks) {
                if (task.command_key.empty() || !runtime.hasCard(actor_key, task.required_card_key)) continue;
                CampCommandOptionView option;
                option.option_id = "camp:task:" + task.key + ":" + actor_key + ":" + std::to_string(state.version);
                option.command_key = task.command_key;
                option.label_text = "让" + actor.name + task.label;
                option.command_kind = taskKind(task);
                option.target_actor_key = actor_key;
                option.target_entity_id = actor_key;
                options.push_back(std::move(option));
            }
        }
    }

    bool canHandle(const CampCommandOptionView& option) const override {
        return option.command_key.rfind("camp.npc_", 0) == 0;
    }

    bool execute(CampRuntime& runtime, const CampCommandOptionView& option, std::string& error) const override {
        const auto actor_key = option.target_actor_key;
        if (runtime.state().actors.find(actor_key) == runtime.state().actors.end()) {
            error = "找不到 NPC";
            return false;
        }
        const CampTaskDefinition* definition = nullptr;
        for (const auto& [_, task] : runtime.content().tasks) {
            if (task.command_key == option.command_key) { definition = &task; break; }
        }
        if (!definition) {
            error = "未知 NPC 任务";
            return false;
        }
        if (!runtime.hasCard(actor_key, definition->required_card_key)) {
            error = "NPC 缺少知识卡：" + definition->required_card_key;
            return false;
        }
        if (!definition->resource_key.empty()) {
            const auto pool_it = runtime.state().resources.find(definition->resource_key);
            const auto resource_it = runtime.content().resources.find(definition->resource_key);
            if (pool_it == runtime.state().resources.end() || pool_it->second.current <= 0) {
                error = resource_it == runtime.content().resources.end() ? "资源不足" : resource_it->second.depleted_text;
                return false;
            }
        }
        runtime.startNpcTask(actor_key, makeTask(*definition));
        return true;
    }
};

} // namespace

std::unique_ptr<CampRule> makeNpcTaskRule() { return std::make_unique<NpcTaskRule>(); }

} // namespace pathfinder::camp
