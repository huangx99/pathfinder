#include "pathfinder/camp/camp_runtime.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace pathfinder::camp {

namespace {

CampKnowledgeView cardKnowledge(const CampKnowledgeCard& card, const std::string& actor_key, const std::string& owner_label) {
    CampKnowledgeView view;
    view.knowledge_id = "card:" + card.card_id + ":" + (actor_key.empty() ? "deck" : actor_key);
    view.subject_id = card.knowledge_key;
    view.action_key = "knowledge_card";
    view.effect_key = "grant_usable_knowledge";
    view.status = card.status;
    view.subject_name = card.name;
    view.action_name = "分配";
    view.effect_name = "可用知识";
    view.summary_text = card.name + "：" + card.summary + "（" + owner_label + "）";
    view.actor_key = actor_key;
    return view;
}

std::string graphStatusForIndex(int index, int active_index) {
    if (index < active_index) return "completed";
    if (index == active_index) return "active";
    return "locked";
}

CampTaskGraphNodeView graphNode(const std::string& id, const std::string& label, const std::string& type, float x, float y, const std::string& status) {
    CampTaskGraphNodeView node;
    node.node_id = id;
    node.label = label;
    node.node_type = type;
    node.x = x;
    node.y = y;
    node.status = status;
    return node;
}

CampTaskGraphEdgeView graphEdge(const std::string& from, const std::string& to, const std::string& type, const std::string& status) {
    CampTaskGraphEdgeView edge;
    edge.edge_id = from + "->" + to;
    edge.from_node_id = from;
    edge.to_node_id = to;
    edge.edge_type = type;
    edge.status = status;
    return edge;
}

std::string nodeLabel(const CampNpcTask& task, int index, const std::string& fallback) {
    if (index >= 0 && index < static_cast<int>(task.nodes.size())) return task.nodes[static_cast<size_t>(index)];
    return fallback;
}

CampTaskGraphView makeTaskGraph(const CampNpcTask& task, int active_index) {
    CampTaskGraphView graph;
    graph.graph_mode = task.risk_encountered ? "branching" : "linear";

    if (!task.risk_encountered) {
        for (int index = 0; index < static_cast<int>(task.nodes.size()); ++index) {
            const std::string id = "main_" + std::to_string(index);
            const std::string type = index == 1 ? "check" : (index + 1 == static_cast<int>(task.nodes.size()) ? "merge" : "action");
            graph.nodes.push_back(graphNode(id, task.nodes[static_cast<size_t>(index)], type, static_cast<float>(index), 0.0F, graphStatusForIndex(index, active_index)));
            if (index > 0) graph.edges.push_back(graphEdge("main_" + std::to_string(index - 1), id, "main", index <= active_index ? "completed" : "locked"));
        }
        graph.active_node_id = "main_" + std::to_string(std::clamp(active_index, 0, std::max(0, static_cast<int>(task.nodes.size()) - 1)));
        return graph;
    }

    const bool resolved_branch = task.risk_resolved;
    const bool success_branch = task.handling_threat || resolved_branch;
    const bool branch_in_progress = task.handling_threat || task.returning_home;
    const std::vector<std::string> main_labels = success_branch
        ? std::vector<std::string>{nodeLabel(task, 0, "接令"), nodeLabel(task, 1, "检查知识"), nodeLabel(task, 2, "圈外执行"), task.risk_resolved_node_label.empty() ? "风险已处理" : task.risk_resolved_node_label, nodeLabel(task, 3, "执行"), nodeLabel(task, 4, "返回交付")}
        : std::vector<std::string>{nodeLabel(task, 0, "接令"), nodeLabel(task, 1, "检查知识"), nodeLabel(task, 2, "圈外执行"), task.risk_return_node_label.empty() ? "返回家园" : task.risk_return_node_label};
    const int branch_active = resolved_branch && !branch_in_progress ? 3 : std::clamp(active_index - 3, 0, 3);
    const int active_main = [&]() {
        if (branch_in_progress) return branch_active >= 3 ? 3 : 2;
        if (resolved_branch) return std::clamp(active_index, 3, static_cast<int>(main_labels.size()) - 1);
        return 2;
    }();
    auto main_status = [&](int index) {
        if (branch_in_progress && index == 2) return std::string("completed");
        if (branch_in_progress && index > 2) return branch_active >= 3 ? graphStatusForIndex(index, active_main) : std::string("locked");
        return graphStatusForIndex(index, active_main);
    };

    for (int index = 0; index < static_cast<int>(main_labels.size()); ++index) {
        const std::string id = "main_" + std::to_string(index);
        const std::string type = index == 1 ? "check" : (index + 1 == static_cast<int>(main_labels.size()) ? "merge" : "action");
        graph.nodes.push_back(graphNode(id, main_labels[static_cast<size_t>(index)], type, static_cast<float>(index), 0.0F, main_status(index)));
        if (index > 0) graph.edges.push_back(graphEdge("main_" + std::to_string(index - 1), id, "main", index <= active_main ? "completed" : "locked"));
    }

    const std::string reason_label = success_branch ? task.risk_success_reason_label : task.risk_failure_reason_label;
    const std::string outcome_label = success_branch ? task.risk_success_action_label : task.risk_failure_action_label;
    graph.nodes.push_back(graphNode("risk_encounter", task.risk_event_label.empty() ? "遭遇风险" : task.risk_event_label, "event", 2.0F, 1.0F, graphStatusForIndex(0, branch_active)));
    graph.nodes.push_back(graphNode("risk_reason", reason_label.empty() ? "推理" : reason_label, "decision", 3.0F, 1.0F, graphStatusForIndex(1, branch_active)));
    graph.nodes.push_back(graphNode("risk_action", outcome_label.empty() ? "决策" : outcome_label, success_branch ? "recovery" : "fail", 4.0F, 1.0F, graphStatusForIndex(2, branch_active)));
    graph.edges.push_back(graphEdge("main_2", "risk_encounter", "branch", "completed"));
    graph.edges.push_back(graphEdge("risk_encounter", "risk_reason", "branch", branch_active >= 1 ? "completed" : "active"));
    graph.edges.push_back(graphEdge("risk_reason", "risk_action", "branch", branch_active >= 2 ? "completed" : "locked"));
    graph.edges.push_back(graphEdge("risk_action", "main_3", success_branch ? "merge" : "return", branch_active >= 3 ? "completed" : "locked"));
    graph.active_node_id = branch_active < 3 ? (branch_active == 0 ? "risk_encounter" : (branch_active == 1 ? "risk_reason" : "risk_action")) : "main_" + std::to_string(active_main);
    return graph;
}

} // namespace

CampRuntime::CampRuntime() : content_(loadDefaultCampContentPack()) {
    reset();
    registerCampCoreRules(*this);
}

void CampRuntime::reset() {
    state_ = CampRuntimeState{};
    seedInitialState();
    last_options_.clear();
}

void CampRuntime::seedInitialState() {
    for (const auto& [key, resource] : content_.resources) {
        state_.resources[key] = CampResourcePool{key, resource.name, resource.current, resource.max, 0, resource.regen_per_day};
    }
    for (const auto& stack : content_.initial_inventory) state_.player_inventory[stack.item_key] += stack.quantity;
    for (const auto& [actor_key, actor] : content_.actors) {
        state_.actors[actor_key] = CampActorState{actor.actor_key, actor.name, actor.x, actor.y, actor.health, actor.max_health, {}, {}};
    }
    for (const auto& [_, card] : content_.knowledge_cards) {
        if (!card.initial) continue;
        state_.cards.push_back(CampKnowledgeCard{card.card_id, card.knowledge_key, card.name, card.summary, card.category, card.status, "", card.recoverable, card.locked});
    }
    if (const auto* threat = primaryThreat()) state_.threat_level = threat->initial_level;
    state_.events = content_.initial_events;
    state_.status_text = content_.status_text;
}

void CampRuntime::registerRule(std::unique_ptr<CampRule> rule) { rules_.push_back(std::move(rule)); }

CampSnapshot CampRuntime::snapshot() {
    auto options = buildOptions();
    last_options_.clear();
    for (const auto& option : options) last_options_[option.option_id] = option;
    return buildSnapshot(options);
}

bool CampRuntime::submitOption(const std::string& option_id, std::string& error) {
    if (last_options_.empty()) {
        auto options = buildOptions();
        for (const auto& option : options) last_options_[option.option_id] = option;
    }
    auto option_it = last_options_.find(option_id);
    if (option_it == last_options_.end()) { error = "Unknown or expired camp option: " + option_id; return false; }
    const auto option = option_it->second;
    if (!option.enabled) { error = "Camp option disabled: " + option.label_text; return false; }
    for (const auto& rule : rules_) {
        if (!rule->canHandle(option)) continue;
        const bool ok = rule->execute(*this, option, error);
        last_options_.clear();
        return ok;
    }
    error = "No camp rule handled option: " + option.command_key;
    return false;
}

bool CampRuntime::assignKnowledgeCard(const std::string& actor_key, const std::string& card_id, std::string& error) {
    CampCommandOptionView option;
    option.command_key = "camp.assign_card." + card_id;
    option.target_actor_key = actor_key;
    option.target_entity_id = actor_key;
    option.command_kind = pathfinder::world_command::WorldCommandKind::Teach;
    for (const auto& rule : rules_) {
        if (!rule->canHandle(option)) continue;
        const bool ok = rule->execute(*this, option, error);
        last_options_.clear();
        return ok;
    }
    error = "No camp rule handled card assignment";
    return false;
}

bool CampRuntime::recoverKnowledgeCard(const std::string& actor_key, const std::string& card_id, std::string& error) {
    CampCommandOptionView option;
    option.command_key = "camp.recover_card." + card_id;
    option.target_actor_key = actor_key;
    option.target_entity_id = actor_key;
    option.command_kind = pathfinder::world_command::WorldCommandKind::Teach;
    for (const auto& rule : rules_) {
        if (!rule->canHandle(option)) continue;
        const bool ok = rule->execute(*this, option, error);
        last_options_.clear();
        return ok;
    }
    error = "No camp rule handled card recovery";
    return false;
}

std::vector<CampCommandOptionView> CampRuntime::buildOptions() const {
    std::vector<CampCommandOptionView> options;
    for (const auto& rule : rules_) rule->buildOptions(*this, options);
    for (size_t index = 0; index < options.size(); ++index) {
        if (options[index].option_id.empty()) options[index].option_id = options[index].command_key + "#" + std::to_string(index) + "@" + std::to_string(state_.version);
    }
    return options;
}

CampSnapshot CampRuntime::buildSnapshot(const std::vector<CampCommandOptionView>& options) const {
    CampSnapshot snapshot;
    snapshot.projection_version = state_.version;
    snapshot.status_text = state_.status_text;
    snapshot.options = options;
    snapshot.player_inventory_capacity_slots = content_.player_inventory_capacity_slots;

    for (int y = -4; y <= 4; ++y) {
        for (int x = -6; x <= 6; ++x) {
            const int dist = std::abs(x) + std::abs(y);
            CampCellView cell;
            cell.cell_id = "cell:" + std::to_string(x) + ":" + std::to_string(y);
            cell.x = x;
            cell.y = y;
            cell.terrain_key = dist > 7 ? "camp_forest_edge" : (dist > 5 ? "camp_boundary" : "camp_grass");
            snapshot.cells.push_back(std::move(cell));
        }
    }

    for (const auto& entity : content_.static_entities) {
        snapshot.entities.push_back(CampEntityView{entity.entity_id, entity.entity_key, entity.display_name, "", entity.x, entity.y, entity.is_resource_node, entity.health, entity.max_health, entity.alive});
    }
    for (const auto& [key, resource] : content_.resources) {
        const auto runtime_it = state_.resources.find(key);
        const int current = runtime_it == state_.resources.end() ? resource.current : runtime_it->second.current;
        snapshot.entities.push_back(CampEntityView{resource.entity_id, resource.entity_key, resource.name + "（剩余 " + std::to_string(current) + "）", "", resource.x, resource.y, true, 0, 0, true});
    }
    if (const auto* threat = primaryThreat(); threat && state_.threat_level > 0) {
        snapshot.entities.push_back(CampEntityView{threat->entity_id, threat->entity_key, threat->name + "踪迹（压力 " + std::to_string(state_.threat_level) + "）", "", threat->x, threat->y, false, threat->health, threat->max_health, true});
    }

    for (const auto& [actor_key, actor] : state_.actors) {
        const auto actor_def = content_.actors.find(actor_key);
        const std::string actor_entity_key = actor_def == content_.actors.end() ? actor_key : actor_def->second.entity_key;
        snapshot.entities.push_back(CampEntityView{actor_key, actor_entity_key, actor.name, actor_key, actor.x, actor.y, false, actor.health, actor.max_health, actor.health > 0});
        std::vector<CampInventoryItemView> items;
        for (const auto& [key, count] : actor.inventory) if (count > 0) items.push_back(makeItem(actor_key, key, count));
        snapshot.actor_inventory[actor_key] = std::move(items);
        snapshot.actor_inventory_capacity_slots[actor_key] = content_.actor_inventory_capacity_slots;
        const bool has_task_projection = actor.task.active || !actor.task.task_key.empty();
        if (has_task_projection) {
            const int phase_percent = std::clamp(actor.task.progress * 100 / std::max(1, actor.task.duration), 0, 100);
            const int node_count = static_cast<int>(std::max<size_t>(1, actor.task.nodes.size()));
            const int max_node_index = std::max(0, node_count - 1);
            int view_node_index = std::clamp(actor.task.node_index, 0, max_node_index);
            int visual_percent = actor.task.active ? phase_percent : 100;
            if (actor.task.active && (actor.task.handling_threat || actor.task.returning_home) && max_node_index > 0) {
                const float phase_ratio = static_cast<float>(phase_percent) / 100.0F;
                const int start_node_index = std::clamp(actor.task.node_index, 0, max_node_index);
                const int remaining_node_count = std::max(0, max_node_index - start_node_index);
                const int phase_node_offset = phase_percent >= 100 ? remaining_node_count : std::clamp(static_cast<int>(std::floor(phase_ratio * static_cast<float>(remaining_node_count + 1))), 0, remaining_node_count);
                view_node_index = std::clamp(start_node_index + phase_node_offset, 0, max_node_index);
                visual_percent = std::clamp(static_cast<int>(std::round((static_cast<float>(start_node_index) + phase_ratio * static_cast<float>(remaining_node_count + 1)) * 100.0F / static_cast<float>(max_node_index))), 0, 100);
            } else if (actor.task.active && max_node_index > 0) {
                view_node_index = std::clamp(static_cast<int>(std::floor(static_cast<float>(phase_percent) * static_cast<float>(max_node_index) / 100.0F)), 0, max_node_index);
                if (actor.task.risk_resolved) view_node_index = std::max(view_node_index, std::clamp(actor.task.node_index, 0, max_node_index));
            } else {
                view_node_index = max_node_index;
            }
            const int graph_node_index = actor.task.active ? view_node_index : max_node_index + 1;
            CampActorTaskView task_view;
            task_view.active = actor.task.active;
            task_view.task_key = actor.task.task_key;
            task_view.label = actor.task.label;
            task_view.nodes = actor.task.nodes;
            task_view.node_index = std::clamp(view_node_index, 0, max_node_index);
            task_view.progress = actor.task.progress;
            task_view.duration = actor.task.duration;
            task_view.percent = visual_percent;
            task_view.outside_safe_zone = actor.task.outside_safe_zone;
            task_view.handling_threat = actor.task.handling_threat;
            task_view.returning_home = actor.task.returning_home;
            task_view.risk_encountered = actor.task.risk_encountered;
            task_view.risk_resolved = actor.task.risk_resolved;
            task_view.required_card_key = actor.task.required_card_key;
            task_view.blocked_reason = actor.task.blocked_reason;
            task_view.risk_text = actor.task.risk_text;
            task_view.reasoning_text = actor.task.reasoning_text;
            task_view.task_graph = makeTaskGraph(actor.task, graph_node_index);
            snapshot.actor_tasks[actor_key] = std::move(task_view);
            const int current_node_index = std::clamp(view_node_index, 0, max_node_index);
            const std::string current_node = actor.task.nodes.empty() ? actor.task.label : actor.task.nodes[static_cast<size_t>(current_node_index)];
            std::ostringstream oss;
            oss << actor.task.label << "  " << current_node << " " << visual_percent << "%";
            if (!actor.task.risk_text.empty()) oss << " · " << actor.task.risk_text;
            if (!actor.task.reasoning_text.empty()) oss << " · " << actor.task.reasoning_text;
            snapshot.actor_work_status[actor_key] = actor.task.active ? oss.str() : (actor.task.blocked_reason.empty() ? ("已完成：" + actor.task.label) : actor.task.blocked_reason);
        } else {
            snapshot.actor_work_status[actor_key] = "待命";
        }
    }

    for (const auto& [key, count] : state_.player_inventory) if (count > 0) snapshot.player_inventory.push_back(makeItem("camp", key, count));
    for (const auto& card : state_.cards) {
        CampKnowledgeCardView card_view;
        card_view.card_id = card.card_id;
        card_view.knowledge_key = card.knowledge_key;
        card_view.name = card.name;
        card_view.summary = card.summary;
        card_view.status = card.status;
        card_view.category = card.category;
        card_view.assigned_actor_key = card.assigned_actor_key;
        card_view.assigned_actor_name = card.assigned_actor_key.empty() ? "" : actorName(card.assigned_actor_key);
        card_view.recoverable = card.recoverable;
        card_view.locked = card.locked;
        snapshot.knowledge_cards.push_back(std::move(card_view));
        snapshot.knowledge.push_back(cardKnowledge(card, "camp", cardOwnerLabel(card)));
        if (!card.assigned_actor_key.empty()) snapshot.knowledge.push_back(cardKnowledge(card, card.assigned_actor_key, "持卡"));
    }
    const size_t start = state_.events.size() > 10 ? state_.events.size() - 10 : 0;
    for (size_t i = start; i < state_.events.size(); ++i) snapshot.event_feed.push_back(state_.events[i]);
    return snapshot;
}

void CampRuntime::markChanged(const std::string& status) { ++state_.version; state_.status_text = status; }

void CampRuntime::addEvent(const std::string& text, const std::string&, const std::string&) {
    state_.events.push_back(text);
    if (state_.events.size() > 30) state_.events.erase(state_.events.begin(), state_.events.begin() + static_cast<long>(state_.events.size() - 30));
}

bool CampRuntime::hasCard(const std::string& actor_key, const std::string& knowledge_key) const {
    return std::any_of(state_.cards.begin(), state_.cards.end(), [&](const CampKnowledgeCard& card) { return card.knowledge_key == knowledge_key && card.assigned_actor_key == actor_key; });
}

bool CampRuntime::hasItem(const std::string& actor_key, const std::string& item_key) const { return itemCount(actor_key, item_key) > 0; }

int CampRuntime::itemCount(const std::string& actor_key, const std::string& item_key) const {
    if (actor_key == "player" || actor_key == "camp" || actor_key.empty()) {
        auto it = state_.player_inventory.find(item_key);
        return it == state_.player_inventory.end() ? 0 : it->second;
    }
    auto actor_it = state_.actors.find(actor_key);
    if (actor_it == state_.actors.end()) return 0;
    auto item_it = actor_it->second.inventory.find(item_key);
    return item_it == actor_it->second.inventory.end() ? 0 : item_it->second;
}

void CampRuntime::addItem(const std::string& actor_key, const std::string& item_key, int amount) {
    if (amount <= 0) return;
    if (actor_key == "player" || actor_key == "camp" || actor_key.empty()) state_.player_inventory[item_key] += amount;
    else state_.actors[actor_key].inventory[item_key] += amount;
}

bool CampRuntime::consumeItem(const std::string& actor_key, const std::string& item_key, int amount) {
    if (amount <= 0) return true;
    auto& inv = (actor_key == "player" || actor_key == "camp" || actor_key.empty()) ? state_.player_inventory : state_.actors[actor_key].inventory;
    auto it = inv.find(item_key);
    if (it == inv.end() || it->second < amount) return false;
    it->second -= amount;
    if (it->second <= 0) inv.erase(it);
    return true;
}

bool CampRuntime::hasItems(const std::string& actor_key, const std::vector<CampItemStackDefinition>& stacks) const {
    return std::all_of(stacks.begin(), stacks.end(), [&](const CampItemStackDefinition& stack) { return itemCount(actor_key, stack.item_key) >= stack.quantity; });
}

bool CampRuntime::consumeItems(const std::string& actor_key, const std::vector<CampItemStackDefinition>& stacks) {
    if (!hasItems(actor_key, stacks)) return false;
    for (const auto& stack : stacks) consumeItem(actor_key, stack.item_key, stack.quantity);
    return true;
}

void CampRuntime::addItems(const std::string& actor_key, const std::vector<CampItemStackDefinition>& stacks) {
    for (const auto& stack : stacks) addItem(actor_key, stack.item_key, stack.quantity);
}

void CampRuntime::startNpcTask(const std::string& actor_key, CampNpcTask task) {
    auto it = state_.actors.find(actor_key);
    if (it == state_.actors.end()) return;
    const auto* threat = findThreat(task.threat_key);
    if (task.outside_safe_zone && threat && !threat->counter_knowledge_key.empty() && hasCard(actor_key, threat->counter_knowledge_key) && !hasItem(actor_key, threat->counter_item_key)) {
        if (consumeItem("camp", threat->counter_item_key, threat->counter_item_quantity)) {
            addItem(actor_key, threat->counter_item_key, threat->counter_item_quantity);
            addEvent(it->second.name + "根据知识卡从营地仓库取走" + makeItem("camp", threat->counter_item_key, 1).display_name + "，准备带出安全圈。", actor_key, "NpcPackedItem");
        } else {
            addEvent(it->second.name + "知道反制方式，但营地仓库缺少所需物品。", actor_key, "NpcMissingItem");
        }
    }
    it->second.task = std::move(task);
    it->second.task.active = true;
    addEvent(it->second.name + "开始任务：" + it->second.task.label, actor_key, "NpcTaskStarted");
    markChanged(it->second.name + "开始" + it->second.task.label);
}

void CampRuntime::advanceOneTick() {
    ++state_.tick;
    ++state_.day_tick;
    if (const auto* threat = primaryThreat(); threat && threat->increase_interval_ticks > 0 && state_.tick % static_cast<uint64_t>(threat->increase_interval_ticks) == 0 && state_.threat_level < threat->max_level) {
        ++state_.threat_level;
        addEvent(threat->name + "重新靠近，威胁上升到 " + std::to_string(state_.threat_level) + "。", {}, "ThreatChanged");
    }
    if (state_.day_tick >= 24) {
        state_.day_tick = 0;
        for (auto& [_, pool] : state_.resources) pool.current = std::min(pool.max, pool.current + pool.regen_per_day);
        addEvent("新的一天到来，圈外资源缓慢恢复。", {}, "ResourceRegen");
    }
    for (auto& [_, actor] : state_.actors) advanceNpcTask(actor);
    markChanged("营地时间推进");
}

void CampRuntime::advanceNpcTask(CampActorState& actor) {
    auto& task = actor.task;
    if (!task.active) return;
    const auto* threat = findThreat(task.threat_key);
    const int encounter_percent = threat ? threat->encounter_percent : 55;
    const bool has_entered_outer_work = task.progress >= std::max(1, task.duration * encounter_percent / 100);
    if (task.outside_safe_zone && threat && has_entered_outer_work && !task.risk_encountered && !task.handling_threat && !task.returning_home && state_.threat_level >= threat->trigger_level) {
        const bool knows = hasCard(actor.actor_key, threat->counter_knowledge_key);
        const bool has_counter_item = itemCount(actor.actor_key, threat->counter_item_key) >= threat->counter_item_quantity;
        task.risk_encountered = true;
        task.resume_progress = task.progress;
        task.resume_duration = task.duration;
        task.risk_text = threat->risk_text;
        task.risk_event_label = threat->encounter_label;
        task.risk_success_reason_label = threat->success_reason_label;
        task.risk_success_action_label = threat->success_action_label;
        task.risk_resolved_node_label = threat->resolved_node_label;
        task.risk_return_node_label = threat->return_node_label;
        if (!knows || !has_counter_item) {
            task.returning_home = true;
            task.risk_failure_reason_label = knows ? threat->failure_reason_without_item_label : threat->failure_reason_without_knowledge_label;
            task.risk_failure_action_label = threat->failure_action_label;
            task.nodes = {nodeLabel(task, 0, "接令"), nodeLabel(task, 1, "检查知识"), nodeLabel(task, 2, "圈外执行"), threat->encounter_label, task.risk_failure_reason_label, threat->return_node_label};
            task.node_index = 3;
            task.progress = 0;
            task.duration = threat->return_duration;
            task.label = "遭遇风险";
            task.reasoning_text = knows ? threat->missing_item_reasoning_text : threat->missing_knowledge_reasoning_text;
            task.blocked_reason = knows ? threat->missing_item_blocked_reason : threat->missing_knowledge_blocked_reason;
            addEvent(actor.name + "在圈外执行时遭遇" + threat->name + "，开始判断能否处理。", actor.actor_key, "NpcRiskEncountered");
            return;
        }
        task.handling_threat = true;
        consumeItem(actor.actor_key, threat->counter_item_key, threat->counter_item_quantity);
        task.nodes = {nodeLabel(task, 0, "接令"), nodeLabel(task, 1, "检查知识"), nodeLabel(task, 2, "圈外执行"), threat->encounter_label, threat->success_reason_label, threat->success_action_label, "继续任务"};
        task.node_index = 3;
        task.progress = 0;
        task.duration = threat->handle_duration;
        task.label = "遭遇风险";
        task.reasoning_text = threat->success_reasoning_text;
        state_.threat_level = std::max(0, state_.threat_level - threat->repel_amount);
        addEvent(actor.name + "在圈外执行时遭遇" + threat->name + "，开始推理反制方式。", actor.actor_key, "NpcHandledThreat");
        return;
    }
    ++task.progress;
    if (task.progress < task.duration) return;
    if (task.handling_threat) {
        task.handling_threat = false;
        task.label = content_.tasks.count(task.task_key) ? content_.tasks.at(task.task_key).label : task.label;
        task.nodes = {nodeLabel(task, 0, "接令"), nodeLabel(task, 1, "检查知识"), nodeLabel(task, 2, "前往圈外"), task.risk_resolved_node_label.empty() ? "风险已处理" : task.risk_resolved_node_label, "执行", "返回交付"};
        task.node_index = 3;
        task.progress = std::min(task.resume_progress + 1, std::max(1, task.resume_duration - 1));
        task.duration = std::max(1, task.resume_duration);
        task.risk_text = threat ? threat->resolved_text : "风险已处理";
        task.risk_resolved = true;
        task.reasoning_text.clear();
        addEvent(actor.name + "处理了圈外风险，继续原任务。", actor.actor_key, "NpcThreatResolved");
        return;
    }
    if (task.returning_home) {
        task.active = false;
        task.progress = task.duration;
        task.node_index = std::max(0, static_cast<int>(task.nodes.size()) - 1);
        const auto actor_def = content_.actors.find(actor.actor_key);
        if (actor_def != content_.actors.end()) { actor.x = actor_def->second.x; actor.y = actor_def->second.y; }
        addEvent(actor.name + task.blocked_reason + "，已经回到安全圈。", actor.actor_key, "NpcReturnedHome");
        return;
    }
    completeNpcTask(actor);
}

void CampRuntime::completeNpcTask(CampActorState& actor) {
    auto& task = actor.task;
    if (!task.resource_key.empty()) {
        auto resource_def = content_.resources.find(task.resource_key);
        auto pool_it = state_.resources.find(task.resource_key);
        if (resource_def == content_.resources.end() || pool_it == state_.resources.end() || pool_it->second.current <= 0) {
            task.active = false;
            task.blocked_reason = "返回家园：" + (resource_def == content_.resources.end() ? std::string("资源不足") : resource_def->second.depleted_text);
            addEvent(actor.name + task.blocked_reason + "。", actor.actor_key, "NpcWorkBlocked");
            return;
        }
        --pool_it->second.current;
        addItems("camp", resource_def->second.npc_output_items.empty() ? resource_def->second.output_items : resource_def->second.npc_output_items);
        addEvent(actor.name + resource_def->second.completion_event, actor.actor_key, "NpcWorkCompleted");
    } else if (!task.recipe_key.empty()) {
        auto recipe_it = content_.recipes.find(task.recipe_key);
        if (recipe_it == content_.recipes.end() || !hasItems("camp", recipe_it->second.inputs)) {
            task.active = false;
            task.blocked_reason = "返回家园：" + (recipe_it == content_.recipes.end() ? std::string("未知配方") : recipe_it->second.missing_material_text);
            addEvent(actor.name + task.blocked_reason + "。", actor.actor_key, "NpcWorkBlocked");
            return;
        }
        consumeItems("camp", recipe_it->second.inputs);
        addItems("camp", recipe_it->second.outputs);
        addEvent(actor.name + recipe_it->second.completion_event, actor.actor_key, "NpcWorkCompleted");
    }
    task.active = false;
    task.progress = task.duration;
    task.node_index = std::max(0, static_cast<int>(task.nodes.size()) - 1);
    task.blocked_reason = "已完成：" + task.label;
}

CampInventoryItemView CampRuntime::makeItem(const std::string& owner, const std::string& key, int quantity) const {
    CampInventoryItemView item;
    item.entry_id = owner + ":" + key;
    item.entity_key = key;
    const auto it = content_.items.find(key);
    item.display_name = it == content_.items.end() ? key : it->second.display_name;
    item.quantity = quantity;
    if (it != content_.items.end()) item.numeric_states = it->second.numeric_states;
    return item;
}

const CampThreatDefinition* CampRuntime::findThreat(const std::string& key) const {
    if (key.empty()) return primaryThreat();
    auto it = content_.threats.find(key);
    return it == content_.threats.end() ? nullptr : &it->second;
}

const CampThreatDefinition* CampRuntime::primaryThreat() const {
    auto it = content_.threats.find(content_.primary_threat_key);
    if (it != content_.threats.end()) return &it->second;
    return content_.threats.empty() ? nullptr : &content_.threats.begin()->second;
}

std::string CampRuntime::actorName(const std::string& actor_key) const {
    auto it = state_.actors.find(actor_key);
    return it == state_.actors.end() ? actor_key : it->second.name;
}

std::string CampRuntime::cardOwnerLabel(const CampKnowledgeCard& card) const {
    if (card.assigned_actor_key.empty()) return "卡库中";
    return "已分配给" + actorName(card.assigned_actor_key);
}

} // namespace pathfinder::camp
