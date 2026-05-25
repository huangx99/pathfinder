#include "pathfinder/v3_sandbox/v3_sandbox_runtime.h"

#include "pathfinder/content/json_content_loader.h"
#include "json.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/knowledge/knowledge_types.h"

namespace pathfinder::v3_sandbox {
namespace {

std::string readTextFile(const std::string& file_path) {
    std::ifstream input(file_path);
    if (!input) return {};
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::vector<std::string> stringArray(const nlohmann::json& json_value) {
    std::vector<std::string> values;
    if (!json_value.is_array()) return values;
    for (const auto& item : json_value) {
        if (item.is_string()) values.push_back(item.get<std::string>());
    }
    return values;
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

int distanceManhattan(V3Coord left, V3Coord right) {
    return std::abs(left.x - right.x) + std::abs(left.y - right.y);
}

std::string statusForEvidence(int evidence_count) {
    if (evidence_count >= 2) return "active";
    return "candidate";
}

std::string attemptKey(const std::string& object_key, const std::string& action_key) {
    return object_key + "|" + action_key;
}

std::string defaultContentRoot() {
#ifdef PATHFINDER_CONTENT_ROOT
    return PATHFINDER_CONTENT_ROOT;
#else
    return "content";
#endif
}

} // namespace

std::shared_ptr<const pathfinder::content::ContentRegistry> V3SandboxRuntime::loadCoreContent(const std::string& content_root) {
    pathfinder::content::JsonContentLoader loader;
    pathfinder::content::ContentLoadOptions options;
    options.root_path = content_root.empty() ? defaultContentRoot() : content_root;
    options.enabled_package_keys = {"core"};
    options.load_mode = pathfinder::content::ContentLoadMode::AllowBuiltinFallback;
    auto loaded = loader.load(options);
    if (loaded.is_error()) return nullptr;
    return loaded.value().registry;
}

V3SandboxConfig V3SandboxRuntime::loadConfigFile(const std::string& file_path) {
    V3SandboxConfig config;
    const auto text = readTextFile(file_path);
    if (text.empty()) return config;

    const auto json_value = nlohmann::json::parse(text);
    if (json_value.contains("map")) {
        const auto& map = json_value.at("map");
        config.width = map.value("width", config.width);
        config.height = map.value("height", config.height);
        config.default_terrain = map.value("default_terrain", config.default_terrain);
    }

    if (json_value.contains("terrains") && json_value.at("terrains").is_array()) {
        config.terrains.clear();
        for (const auto& terrain : json_value.at("terrains")) {
            V3TerrainDefinition definition;
            definition.terrain_key = terrain.value("terrain_key", "");
            definition.display_name = terrain.value("display_name", definition.terrain_key);
            definition.walkable = terrain.value("walkable", true);
            if (!definition.terrain_key.empty()) config.terrains[definition.terrain_key] = definition;
        }
    }

    if (json_value.contains("initial_unlocks")) {
        const auto& unlocks = json_value.at("initial_unlocks");
        for (const auto& key : stringArray(unlocks.value("terrains", nlohmann::json::array()))) {
            config.initial_terrain_unlocks.insert(key);
        }
        for (const auto& key : stringArray(unlocks.value("objects", nlohmann::json::array()))) {
            config.initial_object_unlocks.insert(key);
        }
        for (const auto& key : stringArray(unlocks.value("agents", nlohmann::json::array()))) {
            config.initial_agent_unlocks.insert(key);
        }
    }

    if (json_value.contains("unlock_rules") && json_value.at("unlock_rules").is_array()) {
        config.unlock_rules.clear();
        for (const auto& rule : json_value.at("unlock_rules")) {
            V3UnlockRuleDefinition definition;
            definition.unlock_key = rule.value("unlock_key", "");
            definition.when_any_effect_known = rule.value("when_any_effect_known", "");
            definition.unlock_object = rule.value("unlock_object", "");
            definition.event_text = rule.value("event_text", "");
            if (!definition.unlock_key.empty()) config.unlock_rules.push_back(definition);
        }
    }

    if (json_value.contains("npc_policy")) {
        const auto& policy = json_value.at("npc_policy");
        config.perception_radius = policy.value("perception_radius", config.perception_radius);
        config.hunger_per_tick = policy.value("hunger_per_tick", config.hunger_per_tick);
        config.curiosity_chance_percent = policy.value("curiosity_chance_percent", config.curiosity_chance_percent);
        config.critical_hunger = policy.value("critical_hunger", config.critical_hunger);
        config.health_damage_scale = policy.value("health_damage_scale", config.health_damage_scale);
    }

    return config;
}

V3SandboxRuntime V3SandboxRuntime::createDefault(const std::string& content_root) {
    const auto root = content_root.empty() ? defaultContentRoot() : content_root;
    auto content = loadCoreContent(root);
    auto config = loadConfigFile(root + "/core/v3_sandbox/basic_sandbox.json");
    return V3SandboxRuntime(content, config);
}

V3SandboxRuntime::V3SandboxRuntime(std::shared_ptr<const pathfinder::content::ContentRegistry> content,
                                   V3SandboxConfig config)
    : content_(std::move(content)), config_(std::move(config)) {
    if (config_.terrains.empty()) {
        config_.terrains["grass"] = {"grass", "草地", true};
    }
    if (config_.width <= 0) config_.width = 16;
    if (config_.height <= 0) config_.height = 12;
    cells_.resize(static_cast<size_t>(config_.width * config_.height));
    for (auto& cell : cells_) cell.terrain_key = config_.default_terrain;
    unlocked_terrains_ = config_.initial_terrain_unlocks;
    unlocked_objects_ = config_.initial_object_unlocks;
    unlocked_agents_ = config_.initial_agent_unlocks;
    event_log_.push_back("V3 沙盒已创建：玩家是观察者，NPC 只会根据附近感知和自身经历行动。");
}

V3SandboxCommandResult V3SandboxRuntime::submit(const V3SandboxCommand& command) {
    switch (command.kind) {
        case V3SandboxCommandKind::PaintTerrain: return paintTerrain(command);
        case V3SandboxCommandKind::PlaceObject: return placeObject(command);
        case V3SandboxCommandKind::PlaceAgent: return placeAgent(command);
        case V3SandboxCommandKind::Tick: return advanceTicks(command.tick_count);
        case V3SandboxCommandKind::InspectAgent: return inspectAgent(command);
        case V3SandboxCommandKind::InspectCell: return inspectCell(command);
    }
    return {false, "unknown_command", {}};
}

V3SandboxCommandResult V3SandboxRuntime::paintTerrain(const V3SandboxCommand& command) {
    if (!inBounds(command.x, command.y)) return {false, "out_of_bounds", {}};
    if (!unlocked_terrains_.count(command.terrain_key)) return {false, "terrain_locked", {}};
    if (!config_.terrains.count(command.terrain_key)) return {false, "terrain_unknown", {}};
    cellAt(command.x, command.y).terrain_key = command.terrain_key;
    std::vector<std::string> events = {"绘制地形：" + terrainDisplayName(command.terrain_key) + " @(" + std::to_string(command.x) + "," + std::to_string(command.y) + ")"};
    event_log_.insert(event_log_.end(), events.begin(), events.end());
    return {true, "", events};
}

V3SandboxCommandResult V3SandboxRuntime::placeObject(const V3SandboxCommand& command) {
    if (!inBounds(command.x, command.y)) return {false, "out_of_bounds", {}};
    if (!isWalkable(command.x, command.y)) return {false, "cell_not_walkable", {}};
    if (!unlocked_objects_.count(command.object_key)) return {false, "object_locked", {}};
    if (!content_ || !content_->findObject(command.object_key)) return {false, "object_unknown_in_content_registry", {}};

    ObjectState object;
    object.instance_id = "obj_" + std::to_string(next_object_id_++);
    object.object_key = command.object_key;
    object.quantity = 1;
    object.position = {command.x, command.y};
    objects_[object.instance_id] = object;
    cellAt(command.x, command.y).object_instance_ids.push_back(object.instance_id);

    std::vector<std::string> events = {"投放对象：" + objectDisplayName(command.object_key) + " @(" + std::to_string(command.x) + "," + std::to_string(command.y) + ")"};
    event_log_.insert(event_log_.end(), events.begin(), events.end());
    return {true, "", events};
}

V3SandboxCommandResult V3SandboxRuntime::placeAgent(const V3SandboxCommand& command) {
    if (!inBounds(command.x, command.y)) return {false, "out_of_bounds", {}};
    if (!isWalkable(command.x, command.y)) return {false, "cell_not_walkable", {}};
    if (!unlocked_agents_.count(command.agent_template_key)) return {false, "agent_locked", {}};

    AgentState agent;
    agent.agent_id = "npc_" + std::to_string(next_agent_id_++);
    agent.name = command.agent_name.empty() ? ("小人" + std::to_string(next_agent_id_ - 1)) : command.agent_name;
    agent.position = {command.x, command.y};
    agents_[agent.agent_id] = agent;
    cellAt(command.x, command.y).agent_ids.push_back(agent.agent_id);

    std::vector<std::string> events = {"投放无知 NPC：" + agent.name + " @(" + std::to_string(command.x) + "," + std::to_string(command.y) + ")"};
    event_log_.insert(event_log_.end(), events.begin(), events.end());
    return {true, "", events};
}

V3SandboxCommandResult V3SandboxRuntime::advanceTicks(int count) {
    if (count <= 0) return {false, "invalid_tick_count", {}};
    std::vector<std::string> events;
    for (int i = 0; i < count; ++i) advanceOneTick(events);
    event_log_.insert(event_log_.end(), events.begin(), events.end());
    return {true, "", events};
}

V3SandboxCommandResult V3SandboxRuntime::inspectAgent(const V3SandboxCommand& command) const {
    auto found = agents_.find(command.agent_id);
    if (found == agents_.end()) return {false, "agent_not_found", {}};
    std::vector<std::string> events;
    events.push_back(found->second.name + "：饥饿 " + std::to_string(static_cast<int>(found->second.hunger)) + "，健康 " + std::to_string(static_cast<int>(found->second.health)));
    for (const auto& claim : found->second.knowledge) {
        events.push_back("知识 " + claim.subject_object_key + " + " + claim.action_key + " -> " + claim.effect_key + " [" + statusForEvidence(claim.evidence_count) + "]");
    }
    return {true, "", events};
}

V3SandboxCommandResult V3SandboxRuntime::inspectCell(const V3SandboxCommand& command) const {
    if (!inBounds(command.x, command.y)) return {false, "out_of_bounds", {}};
    const auto& cell = cellAt(command.x, command.y);
    std::vector<std::string> events;
    events.push_back("格子 (" + std::to_string(command.x) + "," + std::to_string(command.y) + ")：" + terrainDisplayName(cell.terrain_key));
    for (const auto& id : cell.object_instance_ids) {
        auto object = objects_.find(id);
        if (object != objects_.end()) events.push_back("对象：" + objectDisplayName(object->second.object_key));
    }
    for (const auto& id : cell.agent_ids) {
        auto agent = agents_.find(id);
        if (agent != agents_.end()) events.push_back("NPC：" + agent->second.name);
    }
    return {true, "", events};
}

void V3SandboxRuntime::advanceOneTick(std::vector<std::string>& events) {
    ++tick_;
    std::vector<std::string> agent_ids;
    for (const auto& [id, agent] : agents_) agent_ids.push_back(id);
    for (const auto& id : agent_ids) {
        auto found = agents_.find(id);
        if (found != agents_.end()) advanceAgent(found->second, events);
    }
    evaluateUnlocks(events);
}

void V3SandboxRuntime::advanceAgent(AgentState& agent, std::vector<std::string>& events) {
    agent.hunger = std::min(100.0, agent.hunger + config_.hunger_per_tick);
    if (agent.health <= 0.0) {
        agent.current_intent = "倒下了，暂时无法行动。";
        return;
    }

    const auto perceived = perceivedObjectIds(agent);
    const V3AgentDecisionPolicy policy;
    const auto decision = policy.decide(buildDecisionContext(agent, perceived));
    if (executeDecision(agent, decision, events)) return;

    wander(agent, events);
}

V3DecisionContext V3SandboxRuntime::buildDecisionContext(const AgentState& agent, const std::vector<std::string>& perceived_object_ids) const {
    V3DecisionContext context;
    context.hunger = agent.hunger;
    context.critical_hunger = config_.critical_hunger;
    context.inventory_slots_used = static_cast<int>(agent.inventory.size());
    context.inventory_capacity_slots = agent.inventory_capacity_slots;
    context.attempted_actions = agent.attempted_actions;

    for (const auto& claim : agent.knowledge) {
        context.knowledge.push_back({claim.subject_object_key, claim.action_key, claim.effect_key, claim.evidence_count, claim.utility_delta, claim.risk_delta});
    }

    for (const auto& object_id : perceived_object_ids) {
        auto object = objects_.find(object_id);
        if (object == objects_.end()) continue;
        const auto* definition = content_ ? content_->findObject(object->second.object_key) : nullptr;
        if (!definition) continue;
        context.visible_objects.push_back({object->second.instance_id, object->second.object_key, definition->kind, definition->allowed_actions, canPickupObject(object->second), false});
    }

    for (const auto& [object_key, quantity] : agent.inventory) {
        if (quantity <= 0) continue;
        const auto* definition = content_ ? content_->findObject(object_key) : nullptr;
        if (!definition) continue;
        context.carried_objects.push_back({"", object_key, definition->kind, definition->allowed_actions, false, true});
    }

    return context;
}

bool V3SandboxRuntime::executeDecision(AgentState& agent, const V3AgentDecision& decision, std::vector<std::string>& events) {
    switch (decision.kind) {
        case V3DecisionKind::TryVisibleObject: return applyObjectAction(agent, decision.object_instance_id, decision.action_key, events);
        case V3DecisionKind::TryCarriedObject: return applyInventoryAction(agent, decision.object_key, decision.action_key, events);
        case V3DecisionKind::PickupVisibleObject: return pickupObject(agent, decision.object_instance_id, events);
        case V3DecisionKind::Wander: return false;
    }
    return false;
}

std::vector<std::string> V3SandboxRuntime::perceivedObjectIds(const AgentState& agent) const {
    std::vector<std::string> ids;
    for (int y = agent.position.y - config_.perception_radius; y <= agent.position.y + config_.perception_radius; ++y) {
        for (int x = agent.position.x - config_.perception_radius; x <= agent.position.x + config_.perception_radius; ++x) {
            if (!inBounds(x, y)) continue;
            if (distanceManhattan(agent.position, {x, y}) > config_.perception_radius) continue;
            const auto& cell = cellAt(x, y);
            ids.insert(ids.end(), cell.object_instance_ids.begin(), cell.object_instance_ids.end());
        }
    }
    return ids;
}

std::optional<V3SandboxRuntime::FeedbackMatch> V3SandboxRuntime::findFeedback(const std::string& object_key, const std::string& action_key) const {
    if (!content_) return std::nullopt;
    auto matches = content_->feedbacksFor(object_key, action_key);
    if (matches.empty()) return std::nullopt;
    const auto* selected = matches.front();
    for (const auto* match : matches) {
        if (match && (!selected || match->priority > selected->priority)) selected = match;
    }
    if (!selected) return std::nullopt;
    FeedbackMatch feedback;
    feedback.object_key = object_key;
    feedback.action_key = action_key;
    feedback.effect_key = selected->knowledge_effect_key.empty() ? selected->effect_key : selected->knowledge_effect_key;
    feedback.reply_key = selected->reply_key;
    feedback.utility_delta = selected->utility_delta;
    feedback.risk_delta = selected->risk_delta;
    feedback.outcome_signal_keys = selected->outcome_signal_keys;
    return feedback;
}

bool V3SandboxRuntime::applyObjectAction(AgentState& agent, const std::string& object_id, const std::string& action_key, std::vector<std::string>& events) {
    auto object = objects_.find(object_id);
    if (object == objects_.end()) return false;
    if (distanceManhattan(agent.position, object->second.position) > config_.perception_radius) return false;
    agent.attempted_actions.insert(attemptKey(object->second.object_key, action_key));

    auto feedback = findFeedback(object->second.object_key, action_key);
    if (!feedback) {
        agent.current_intent = "尝试" + action_key + " " + objectDisplayName(object->second.object_key) + "，但没有明显反馈。";
        remember(agent, agent.current_intent);
        learnNoVisibleEffect(agent, object->second.object_key, action_key, events);
        events.push_back(agent.name + " 记住了：" + object->second.object_key + " + " + action_key + " -> no_visible_effect。");
        return true;
    }

    agent.current_intent = "尝试" + feedback->action_key + " " + objectDisplayName(object->second.object_key) + "。";
    agent.hunger = std::clamp(agent.hunger - feedback->utility_delta * 80.0, 0.0, 100.0);
    if (feedback->risk_delta > 0.0) agent.health = std::max(0.0, agent.health - feedback->risk_delta * config_.health_damage_scale);
    learn(agent, *feedback, events);

    const bool consumed = feedback->action_key == "eat" || contains(feedback->outcome_signal_keys, "object_consumed");
    if (consumed) {
        auto& cell = cellAt(object->second.position.x, object->second.position.y);
        cell.object_instance_ids.erase(std::remove(cell.object_instance_ids.begin(), cell.object_instance_ids.end(), object_id), cell.object_instance_ids.end());
        objects_.erase(object);
    }

    const auto text = agent.name + " 在附近尝试 " + feedback->action_key + " " + feedback->object_key + "，得到反馈 " + feedback->effect_key + "。";
    remember(agent, text);
    events.push_back(text);
    return true;
}

bool V3SandboxRuntime::applyInventoryAction(AgentState& agent, const std::string& object_key, const std::string& action_key, std::vector<std::string>& events) {
    auto carried = agent.inventory.find(object_key);
    if (carried == agent.inventory.end() || carried->second <= 0) return false;
    agent.attempted_actions.insert(attemptKey(object_key, action_key));

    auto feedback = findFeedback(object_key, action_key);
    if (!feedback) {
        agent.current_intent = "从背包尝试" + action_key + " " + objectDisplayName(object_key) + "，但没有明显反馈。";
        remember(agent, agent.current_intent);
        learnNoVisibleEffect(agent, object_key, action_key, events);
        events.push_back(agent.name + " 记住了：背包中的 " + object_key + " + " + action_key + " -> no_visible_effect。");
        return true;
    }

    agent.current_intent = "从背包尝试" + feedback->action_key + " " + objectDisplayName(object_key) + "。";
    agent.hunger = std::clamp(agent.hunger - feedback->utility_delta * 80.0, 0.0, 100.0);
    if (feedback->risk_delta > 0.0) agent.health = std::max(0.0, agent.health - feedback->risk_delta * config_.health_damage_scale);
    learn(agent, *feedback, events);

    const bool consumed = feedback->action_key == "eat" || contains(feedback->outcome_signal_keys, "object_consumed");
    if (consumed) {
        --carried->second;
        if (carried->second <= 0) agent.inventory.erase(carried);
    }

    const auto text = agent.name + " 使用背包中的 " + feedback->object_key + " 执行 " + feedback->action_key + "，得到反馈 " + feedback->effect_key + "。";
    remember(agent, text);
    events.push_back(text);
    return true;
}

bool V3SandboxRuntime::pickupObject(AgentState& agent, const std::string& object_id, std::vector<std::string>& events) {
    auto object = objects_.find(object_id);
    if (object == objects_.end()) return false;
    if (distanceManhattan(agent.position, object->second.position) > config_.perception_radius) return false;
    if (!canPickupObject(object->second)) return false;
    if (!agent.inventory.count(object->second.object_key) && static_cast<int>(agent.inventory.size()) >= agent.inventory_capacity_slots) return false;

    const auto object_key = object->second.object_key;
    agent.inventory[object_key] += std::max(1, object->second.quantity);
    auto& cell = cellAt(object->second.position.x, object->second.position.y);
    cell.object_instance_ids.erase(std::remove(cell.object_instance_ids.begin(), cell.object_instance_ids.end(), object_id), cell.object_instance_ids.end());
    objects_.erase(object);

    agent.current_intent = "捡起" + objectDisplayName(object_key) + "放进背包。";
    remember(agent, agent.current_intent);
    events.push_back(agent.name + " 捡起了 " + objectDisplayName(object_key) + "。");
    return true;
}

void V3SandboxRuntime::learnNoVisibleEffect(AgentState& agent, const std::string& object_key, const std::string& action_key, std::vector<std::string>& events) {
    FeedbackMatch feedback;
    feedback.object_key = object_key;
    feedback.action_key = action_key;
    feedback.effect_key = "no_visible_effect";
    feedback.utility_delta = 0.0;
    feedback.risk_delta = 0.0;
    learn(agent, feedback, events);
}

void V3SandboxRuntime::remember(AgentState& agent, const std::string& text) {
    agent.recent_memory.push_back("T" + std::to_string(tick_) + " " + text);
    if (agent.recent_memory.size() > 8) agent.recent_memory.erase(agent.recent_memory.begin());
}

void V3SandboxRuntime::learn(AgentState& agent, const FeedbackMatch& feedback, std::vector<std::string>& events) {
    for (auto& claim : agent.knowledge) {
        if (claim.subject_object_key == feedback.object_key && claim.action_key == feedback.action_key && claim.effect_key == feedback.effect_key) {
            ++claim.evidence_count;
            claim.utility_delta = feedback.utility_delta;
            claim.risk_delta = feedback.risk_delta;
            mirrorKnowledgeClaim(agent, claim);
            events.push_back(agent.name + " 强化知识：" + feedback.object_key + " + " + feedback.action_key + " -> " + feedback.effect_key + " [" + statusForEvidence(claim.evidence_count) + "]");
            return;
        }
    }
    KnowledgeClaimState claim;
    claim.subject_object_key = feedback.object_key;
    claim.action_key = feedback.action_key;
    claim.effect_key = feedback.effect_key;
    claim.evidence_count = 1;
    claim.utility_delta = feedback.utility_delta;
    claim.risk_delta = feedback.risk_delta;
    agent.knowledge.push_back(claim);
    mirrorKnowledgeClaim(agent, agent.knowledge.back());
    events.push_back(agent.name + " 形成知识：" + feedback.object_key + " + " + feedback.action_key + " -> " + feedback.effect_key + " [candidate]");
}

void V3SandboxRuntime::mirrorKnowledgeClaim(const AgentState& agent, const KnowledgeClaimState& claim) {
    namespace foundation = pathfinder::foundation;
    namespace knowledge = pathfinder::knowledge;

    knowledge::KnowledgeClaim repository_claim;
    const auto knowledge_key = "v3_" + agent.agent_id + "_" + claim.subject_object_key + "_" + claim.action_key + "_" + claim.effect_key;
    repository_claim.knowledge_id = foundation::KnowledgeId(knowledge_key);
    repository_claim.owner.kind = knowledge::KnowledgeOwnerKind::Agent;
    repository_claim.owner.entity_id = foundation::EntityId(agent.agent_id);
    repository_claim.subject.kind = knowledge::KnowledgeSubjectKind::ObjectDefinition;
    repository_claim.subject.subject_id = claim.subject_object_key;
    repository_claim.subject.subject_type_key = "content_object";
    repository_claim.predicate.relation_type = knowledge::KnowledgeRelationType::HasEffect;
    repository_claim.predicate.action_key = claim.action_key;
    repository_claim.predicate.effect_key = claim.effect_key;
    repository_claim.confidence.confidence = std::min(0.95, 0.25 + static_cast<double>(claim.evidence_count) * 0.25);
    repository_claim.confidence.support_count = static_cast<size_t>(claim.evidence_count);
    repository_claim.confidence.last_verified_tick = foundation::Tick(static_cast<uint64_t>(tick_));
    repository_claim.status = claim.evidence_count >= 2 ? knowledge::KnowledgeStatus::Active : knowledge::KnowledgeStatus::Candidate;
    repository_claim.projection_flags.usable_by_ai = true;
    repository_claim.projection_flags.usable_for_action = claim.risk_delta <= 0.15;
    repository_claim.reason_keys = {"v3_direct_experience"};

    knowledge::KnowledgeEvidence evidence;
    evidence.evidence_id = knowledge::KnowledgeEvidenceId(knowledge_key + "_ev_" + std::to_string(claim.evidence_count));
    evidence.evidence_kind = knowledge::KnowledgeEvidenceKind::DirectExperience;
    evidence.source_memory_ids.push_back(foundation::MemoryId("v3_memory_" + agent.agent_id + "_" + std::to_string(tick_)));
    evidence.confidence_delta = 0.25;
    evidence.reliability = 0.8;
    evidence.observed_tick = foundation::Tick(static_cast<uint64_t>(tick_));
    evidence.reason_keys = {"v3_object_action_feedback"};
    repository_claim.evidence_refs.push_back(evidence);

    auto put_result = knowledge_repository_.put(std::move(repository_claim));
    (void)put_result;
}

void V3SandboxRuntime::evaluateUnlocks(std::vector<std::string>& events) {
    for (const auto& rule : config_.unlock_rules) {
        if (fired_unlock_rules_.count(rule.unlock_key)) continue;
        bool matched = false;
        for (const auto& [id, agent] : agents_) {
            for (const auto& claim : agent.knowledge) {
                if (claim.effect_key == rule.when_any_effect_known) matched = true;
            }
        }
        if (!matched) continue;
        if (!rule.unlock_object.empty() && content_ && content_->findObject(rule.unlock_object)) {
            unlocked_objects_.insert(rule.unlock_object);
        }
        fired_unlock_rules_.insert(rule.unlock_key);
        events.push_back(rule.event_text.empty() ? ("解锁：" + rule.unlock_object) : rule.event_text);
    }
}

void V3SandboxRuntime::moveAgentOneStep(AgentState& agent, V3Coord destination, std::vector<std::string>& events) {
    if (!inBounds(destination.x, destination.y) || !isWalkable(destination.x, destination.y)) return;
    auto& from = cellAt(agent.position.x, agent.position.y);
    from.agent_ids.erase(std::remove(from.agent_ids.begin(), from.agent_ids.end(), agent.agent_id), from.agent_ids.end());
    agent.position = destination;
    cellAt(agent.position.x, agent.position.y).agent_ids.push_back(agent.agent_id);
    agent.current_intent = "只根据附近感知随机探索。";
    events.push_back(agent.name + " 移动到 (" + std::to_string(agent.position.x) + "," + std::to_string(agent.position.y) + ")。");
}

void V3SandboxRuntime::wander(AgentState& agent, std::vector<std::string>& events) {
    static const V3Coord directions[] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    for (int attempt = 0; attempt < 4; ++attempt) {
        const auto direction = directions[(agent.exploration_cursor + attempt) % 4];
        V3Coord next{agent.position.x + direction.x, agent.position.y + direction.y};
        if (inBounds(next.x, next.y) && isWalkable(next.x, next.y)) {
            agent.exploration_cursor = (agent.exploration_cursor + attempt + 1) % 4;
            moveAgentOneStep(agent, next, events);
            return;
        }
    }
    agent.current_intent = "附近没有可走方向，停在原地。";
}

bool V3SandboxRuntime::inBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < config_.width && y < config_.height;
}

bool V3SandboxRuntime::isWalkable(int x, int y) const {
    if (!inBounds(x, y)) return false;
    const auto terrain = config_.terrains.find(cellAt(x, y).terrain_key);
    return terrain == config_.terrains.end() || terrain->second.walkable;
}

bool V3SandboxRuntime::canPickupObject(const ObjectState& object) const {
    const auto* definition = content_ ? content_->findObject(object.object_key) : nullptr;
    if (!definition) return false;
    if (definition->kind == "hazard") return false;
    if (contains(definition->safe_tags, "creature") || contains(definition->safe_tags, "predator")) return false;
    return true;
}

V3SandboxRuntime::CellState& V3SandboxRuntime::cellAt(int x, int y) {
    return cells_[static_cast<size_t>(y * config_.width + x)];
}

const V3SandboxRuntime::CellState& V3SandboxRuntime::cellAt(int x, int y) const {
    return cells_[static_cast<size_t>(y * config_.width + x)];
}

std::string V3SandboxRuntime::objectDisplayName(const std::string& object_key) const {
    if (!content_) return object_key;
    const auto* object = content_->findObject(object_key);
    if (!object) return object_key;
    auto translated = content_->translate("zh_cn", object->display_key);
    return translated.empty() ? object_key : translated;
}

std::string V3SandboxRuntime::terrainDisplayName(const std::string& terrain_key) const {
    auto found = config_.terrains.find(terrain_key);
    if (found == config_.terrains.end()) return terrain_key;
    return found->second.display_name.empty() ? terrain_key : found->second.display_name;
}

bool V3SandboxRuntime::hasKnowledge(const AgentState& agent, const std::string& object_key, const std::string& action_key) const {
    for (const auto& claim : agent.knowledge) {
        if (claim.subject_object_key == object_key && claim.action_key == action_key) return true;
    }
    return false;
}

V3SandboxSnapshot V3SandboxRuntime::snapshot() const {
    V3SandboxSnapshot view;
    view.tick = tick_;
    view.width = config_.width;
    view.height = config_.height;
    for (int y = 0; y < config_.height; ++y) {
        for (int x = 0; x < config_.width; ++x) {
            const auto& cell = cellAt(x, y);
            V3SandboxCellView cell_view;
            cell_view.x = x;
            cell_view.y = y;
            cell_view.terrain_key = cell.terrain_key;
            cell_view.terrain_name = terrainDisplayName(cell.terrain_key);
            cell_view.walkable = isWalkable(x, y);
            cell_view.object_instance_ids = cell.object_instance_ids;
            cell_view.agent_ids = cell.agent_ids;
            view.cells.push_back(cell_view);
        }
    }
    for (const auto& [id, object] : objects_) {
        view.objects.push_back({object.instance_id, object.object_key, objectDisplayName(object.object_key), object.quantity, object.position.x, object.position.y});
    }
    for (const auto& [id, agent] : agents_) {
        V3AgentView agent_view;
        agent_view.agent_id = agent.agent_id;
        agent_view.name = agent.name;
        agent_view.x = agent.position.x;
        agent_view.y = agent.position.y;
        agent_view.hunger = agent.hunger;
        agent_view.health = agent.health;
        agent_view.current_intent = agent.current_intent;
        for (const auto& [object_key, quantity] : agent.inventory) {
            if (quantity > 0) agent_view.inventory.push_back({object_key, objectDisplayName(object_key), quantity});
        }
        agent_view.recent_memory = agent.recent_memory;
        for (const auto& claim : agent.knowledge) {
            agent_view.knowledge.push_back({claim.subject_object_key, claim.action_key, claim.effect_key, statusForEvidence(claim.evidence_count), claim.evidence_count, claim.utility_delta, claim.risk_delta});
        }
        view.agents.push_back(agent_view);
    }
    view.unlocked_terrains.assign(unlocked_terrains_.begin(), unlocked_terrains_.end());
    view.unlocked_objects.assign(unlocked_objects_.begin(), unlocked_objects_.end());
    view.events = event_log_;
    return view;
}

} // namespace pathfinder::v3_sandbox
