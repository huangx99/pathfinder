#include "pathfinder/camp/camp_content.h"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace pathfinder::camp {
namespace {

using json = nlohmann::json;

std::string stringValue(const json& value, const char* key, const std::string& fallback = {}) {
    if (!value.contains(key) || !value.at(key).is_string()) return fallback;
    return value.at(key).get<std::string>();
}

int intValue(const json& value, const char* key, int fallback = 0) {
    if (!value.contains(key) || !value.at(key).is_number_integer()) return fallback;
    return value.at(key).get<int>();
}

bool boolValue(const json& value, const char* key, bool fallback = false) {
    if (!value.contains(key) || !value.at(key).is_boolean()) return fallback;
    return value.at(key).get<bool>();
}

std::vector<std::string> stringArray(const json& value, const char* key) {
    std::vector<std::string> out;
    if (!value.contains(key) || !value.at(key).is_array()) return out;
    for (const auto& item : value.at(key)) {
        if (item.is_string()) out.push_back(item.get<std::string>());
    }
    return out;
}

std::map<std::string, double> numericMap(const json& value, const char* key) {
    std::map<std::string, double> out;
    if (!value.contains(key) || !value.at(key).is_object()) return out;
    for (auto it = value.at(key).begin(); it != value.at(key).end(); ++it) {
        if (it.value().is_number()) out[it.key()] = it.value().get<double>();
    }
    return out;
}

std::vector<CampItemStackDefinition> itemStacks(const json& value, const char* key) {
    std::vector<CampItemStackDefinition> out;
    if (!value.contains(key) || !value.at(key).is_array()) return out;
    for (const auto& item : value.at(key)) {
        CampItemStackDefinition stack;
        stack.item_key = stringValue(item, "item_key");
        stack.quantity = intValue(item, "quantity", 0);
        if (!stack.item_key.empty() && stack.quantity > 0) out.push_back(std::move(stack));
    }
    return out;
}

void requireKey(const std::string& key, const std::string& context) {
    if (key.empty()) throw std::runtime_error("missing key in " + context);
}

std::filesystem::path defaultContentPath() {
#ifdef PATHFINDER_CONTENT_ROOT
    const std::filesystem::path configured = PATHFINDER_CONTENT_ROOT;
    const auto configured_file = configured / "core" / "camp" / "basic_camp.json";
    if (std::filesystem::exists(configured_file)) return configured_file;
#endif
    auto cursor = std::filesystem::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        const auto candidate = cursor / "content" / "core" / "camp" / "basic_camp.json";
        if (std::filesystem::exists(candidate)) return candidate;
        if (!cursor.has_parent_path() || cursor == cursor.parent_path()) break;
        cursor = cursor.parent_path();
    }
    return std::filesystem::path("content/core/camp/basic_camp.json");
}

CampContentPack parsePack(const json& root, const std::string& source_path) {
    CampContentPack pack;
    pack.schema_version = stringValue(root, "schema_version");
    requireKey(pack.schema_version, "camp_content.schema_version");
    pack.source_path = source_path;
    pack.status_text = stringValue(root, "status_text");
    pack.player_inventory_capacity_slots = intValue(root, "player_inventory_capacity_slots", 18);
    pack.actor_inventory_capacity_slots = intValue(root, "actor_inventory_capacity_slots", 9);
    pack.initial_events = stringArray(root, "initial_events");
    pack.initial_inventory = itemStacks(root, "initial_inventory");
    pack.primary_threat_key = stringValue(root, "primary_threat_key");
    pack.scholar_entity_id = stringValue(root, "scholar_entity_id");
    pack.scholar_reward_card_id = stringValue(root, "scholar_reward_card_id");
    pack.scholar_repeat_event = stringValue(root, "scholar_repeat_event", "学者提醒：知识卡要分配给合适的人。");

    for (const auto& value : root.value("items", json::array())) {
        CampItemDefinition item;
        item.key = stringValue(value, "key");
        requireKey(item.key, "camp_content.items");
        item.display_name = stringValue(value, "display_name", item.key);
        item.numeric_states = numericMap(value, "numeric_states");
        pack.items[item.key] = std::move(item);
    }
    for (const auto& value : root.value("static_entities", json::array())) {
        CampStaticEntityDefinition entity;
        entity.entity_id = stringValue(value, "entity_id");
        requireKey(entity.entity_id, "camp_content.static_entities");
        entity.entity_key = stringValue(value, "entity_key");
        entity.display_name = stringValue(value, "display_name", entity.entity_key);
        entity.x = intValue(value, "x");
        entity.y = intValue(value, "y");
        entity.is_resource_node = boolValue(value, "is_resource_node", false);
        entity.health = intValue(value, "health", 0);
        entity.max_health = intValue(value, "max_health", 0);
        entity.alive = boolValue(value, "alive", true);
        pack.static_entities.push_back(std::move(entity));
    }
    for (const auto& value : root.value("resources", json::array())) {
        CampResourceDefinition resource;
        resource.key = stringValue(value, "key");
        requireKey(resource.key, "camp_content.resources");
        resource.name = stringValue(value, "name", resource.key);
        resource.current = intValue(value, "current", 0);
        resource.max = intValue(value, "max", resource.current);
        resource.regen_per_day = intValue(value, "regen_per_day", 0);
        resource.entity_id = stringValue(value, "entity_id", resource.key);
        resource.entity_key = stringValue(value, "entity_key");
        resource.x = intValue(value, "x");
        resource.y = intValue(value, "y");
        resource.gather_command_key = stringValue(value, "gather_command_key");
        resource.gather_label = stringValue(value, "gather_label", resource.name);
        resource.output_items = itemStacks(value, "output_items");
        resource.npc_output_items = itemStacks(value, "npc_output_items");
        resource.depleted_text = stringValue(value, "depleted_text", resource.name + "资源不足");
        resource.completion_event = stringValue(value, "completion_event");
        pack.resources[resource.key] = std::move(resource);
    }
    for (const auto& value : root.value("actors", json::array())) {
        CampActorDefinition actor;
        actor.actor_key = stringValue(value, "actor_key");
        requireKey(actor.actor_key, "camp_content.actors");
        actor.entity_key = stringValue(value, "entity_key");
        requireKey(actor.entity_key, "camp_content.actors.entity_key");
        actor.name = stringValue(value, "name", actor.actor_key);
        actor.x = intValue(value, "x");
        actor.y = intValue(value, "y");
        actor.health = intValue(value, "health", 10);
        actor.max_health = intValue(value, "max_health", actor.health);
        pack.actors[actor.actor_key] = std::move(actor);
    }
    for (const auto& value : root.value("knowledge_cards", json::array())) {
        CampKnowledgeCardDefinition card;
        card.card_id = stringValue(value, "card_id");
        requireKey(card.card_id, "camp_content.knowledge_cards");
        card.knowledge_key = stringValue(value, "knowledge_key");
        card.name = stringValue(value, "name", card.knowledge_key);
        card.summary = stringValue(value, "summary");
        card.category = stringValue(value, "category", "通用");
        card.status = stringValue(value, "status", "active");
        card.initial = boolValue(value, "initial", true);
        card.recoverable = boolValue(value, "recoverable", true);
        card.locked = boolValue(value, "locked", false);
        pack.knowledge_cards[card.card_id] = std::move(card);
    }
    for (const auto& value : root.value("threats", json::array())) {
        CampThreatDefinition threat;
        threat.key = stringValue(value, "key");
        requireKey(threat.key, "camp_content.threats");
        threat.name = stringValue(value, "name", threat.key);
        threat.entity_id = stringValue(value, "entity_id", threat.key);
        threat.entity_key = stringValue(value, "entity_key");
        threat.x = intValue(value, "x");
        threat.y = intValue(value, "y");
        threat.health = intValue(value, "health", 0);
        threat.max_health = intValue(value, "max_health", 0);
        threat.initial_level = intValue(value, "initial_level", 0);
        threat.max_level = intValue(value, "max_level", threat.initial_level);
        threat.trigger_level = intValue(value, "trigger_level", 1);
        threat.increase_interval_ticks = intValue(value, "increase_interval_ticks", 6);
        threat.repel_amount = intValue(value, "repel_amount", 1);
        threat.encounter_percent = intValue(value, "encounter_percent", 55);
        threat.handle_duration = intValue(value, "handle_duration", 12);
        threat.return_duration = intValue(value, "return_duration", 9);
        threat.repel_command_key = stringValue(value, "repel_command_key");
        threat.repel_label = stringValue(value, "repel_label");
        threat.counter_knowledge_key = stringValue(value, "counter_knowledge_key");
        threat.counter_item_key = stringValue(value, "counter_item_key");
        threat.counter_item_quantity = intValue(value, "counter_item_quantity", 1);
        threat.encounter_label = stringValue(value, "encounter_label", threat.name);
        threat.success_reason_label = stringValue(value, "success_reason_label");
        threat.failure_reason_without_item_label = stringValue(value, "failure_reason_without_item_label");
        threat.failure_reason_without_knowledge_label = stringValue(value, "failure_reason_without_knowledge_label");
        threat.success_action_label = stringValue(value, "success_action_label");
        threat.failure_action_label = stringValue(value, "failure_action_label");
        threat.resolved_node_label = stringValue(value, "resolved_node_label", "风险已处理");
        threat.return_node_label = stringValue(value, "return_node_label", "返回家园");
        threat.risk_text = stringValue(value, "risk_text");
        threat.resolved_text = stringValue(value, "resolved_text");
        threat.missing_item_blocked_reason = stringValue(value, "missing_item_blocked_reason");
        threat.missing_knowledge_blocked_reason = stringValue(value, "missing_knowledge_blocked_reason");
        threat.success_reasoning_text = stringValue(value, "success_reasoning_text");
        threat.missing_item_reasoning_text = stringValue(value, "missing_item_reasoning_text");
        threat.missing_knowledge_reasoning_text = stringValue(value, "missing_knowledge_reasoning_text");
        pack.threats[threat.key] = std::move(threat);
    }
    for (const auto& value : root.value("recipes", json::array())) {
        CampRecipeDefinition recipe;
        recipe.key = stringValue(value, "key");
        requireKey(recipe.key, "camp_content.recipes");
        recipe.command_key = stringValue(value, "command_key");
        requireKey(recipe.command_key, "camp_content.recipes.command_key");
        recipe.label = stringValue(value, "label", recipe.key);
        recipe.work_label = stringValue(value, "work_label", recipe.label);
        recipe.target_entity_id = stringValue(value, "target_entity_id");
        recipe.required_card_key = stringValue(value, "required_card_key");
        recipe.duration = intValue(value, "duration", 1);
        recipe.inputs = itemStacks(value, "inputs");
        recipe.outputs = itemStacks(value, "outputs");
        recipe.missing_material_text = stringValue(value, "missing_material_text", "材料不足");
        recipe.completion_event = stringValue(value, "completion_event");
        pack.recipes[recipe.key] = std::move(recipe);
    }
    for (const auto& value : root.value("tasks", json::array())) {
        CampTaskDefinition task;
        task.key = stringValue(value, "key");
        requireKey(task.key, "camp_content.tasks");
        task.command_key = stringValue(value, "command_key");
        requireKey(task.command_key, "camp_content.tasks.command_key");
        task.label = stringValue(value, "label", task.key);
        task.duration = intValue(value, "duration", 1);
        task.outside_safe_zone = boolValue(value, "outside_safe_zone", false);
        task.required_card_key = stringValue(value, "required_card_key");
        task.resource_key = stringValue(value, "resource_key");
        task.recipe_key = stringValue(value, "recipe_key");
        task.threat_key = stringValue(value, "threat_key");
        task.nodes = stringArray(value, "nodes");
        pack.tasks[task.key] = std::move(task);
    }
    if (pack.primary_threat_key.empty() && !pack.threats.empty()) pack.primary_threat_key = pack.threats.begin()->first;
    return pack;
}

} // namespace

CampContentPack loadDefaultCampContentPack() {
    const auto path = defaultContentPath();
    std::ifstream input(path);
    if (!input) throw std::runtime_error("failed to open camp content: " + path.string());
    json root;
    input >> root;
    return parsePack(root, path.string());
}

} // namespace pathfinder::camp
