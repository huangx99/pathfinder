#include "runtime/EngineLocalClient.h"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <sstream>

namespace pf::client {
namespace {

int intField(const std::map<std::string, std::string>& fields, const std::string& key, int fallback = 0) {
    auto it = fields.find(key);
    if (it == fields.end()) return fallback;
    int value = fallback;
    auto begin = it->second.data();
    auto end = it->second.data() + it->second.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc{} ? value : fallback;
}

double doubleField(const std::map<std::string, std::string>& fields, const std::string& key, double fallback = 0.0) {
    auto it = fields.find(key);
    if (it == fields.end()) return fallback;
    char* end = nullptr;
    const double value = std::strtod(it->second.c_str(), &end);
    return end == it->second.c_str() ? fallback : value;
}

bool boolField(const std::map<std::string, std::string>& fields, const std::string& key) {
    auto it = fields.find(key);
    return it != fields.end() && it->second == "true";
}

std::vector<std::string> splitCsv(const std::string& value) {
    std::vector<std::string> result;
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) result.push_back(item);
    }
    return result;
}

std::string displayNameForKey(const std::string& key) {
    if (key == "player") return "探索者";
    if (key == "red_berry") return "红果";
    if (key == "decayed_red_berry") return "腐烂红果";
    if (key == "bitter_leaf") return "苦叶";
    if (key == "stone_flake") return "石片";
    if (key == "loose_stone") return "碎石";
    if (key == "wood") return "木头";
    if (key == "axe") return "斧头";
    if (key == "torch") return "火把";
    if (key == "berry_bush") return "浆果丛";
    if (key == "young_tree") return "幼树";
    if (key == "loose_stone_node") return "碎石堆";
    if (key.empty()) return "未知";
    return key;
}

std::string optionCategory(pathfinder::world_command::WorldCommandKind kind) {
    using pathfinder::world_command::WorldCommandKind;
    switch (kind) {
        case WorldCommandKind::Move: return "移动";
        case WorldCommandKind::Gather:
        case WorldCommandKind::Chop:
        case WorldCommandKind::Mine:
        case WorldCommandKind::Dig: return "采集";
        case WorldCommandKind::Pickup:
        case WorldCommandKind::Drop: return "背包";
        case WorldCommandKind::Craft: return "制作";
        case WorldCommandKind::Eat:
        case WorldCommandKind::Use: return "使用";
        case WorldCommandKind::Inspect: return "观察";
        case WorldCommandKind::Teach: return "教学";
        case WorldCommandKind::Wait: return "时间";
        default: return "命令";
    }
}

std::string eventText(const pathfinder::world_command::WorldEventDto& event) {
    if (!event.title_text.empty() && !event.body_text.empty()) return event.title_text + "：" + event.body_text;
    if (!event.title_text.empty()) return event.title_text;
    if (!event.body_text.empty()) return event.body_text;
    return event.event_kind;
}

} // namespace

EngineLocalClient::EngineLocalClient() {
    bootstrap();
}

void EngineLocalClient::selectTool(int index) {
    if (index < 0 || index >= static_cast<int>(tools_.size())) return;
    selected_tool_index_ = index;
}

void EngineLocalClient::clearToolSelection() {
    selected_tool_index_ = -1;
    last_error_.clear();
}

bool EngineLocalClient::applySelectedToolToCell(int x, int y) {
    if (hasSelectedTool()) {
        return submitOption(tools_[selected_tool_index_].key);
    }

    auto cell_option = findOptionForCell(x, y);
    if (cell_option) return submitOption(cell_option->option_id);

    last_error_ = "no_available_command_for_cell";
    return false;
}

bool EngineLocalClient::tick(int count) {
    bool ok = true;
    for (int i = 0; i < std::max(1, count); ++i) {
        auto wait_option = findOptionByKind(pathfinder::world_command::WorldCommandKind::Wait);
        if (wait_option) {
            ok = submitOption(wait_option->option_id) && ok;
        } else {
            last_error_ = "wait_option_not_available";
            ok = false;
        }
    }
    return ok;
}

bool EngineLocalClient::inspectCell(int x, int y, std::vector<std::string>& lines) {
    const bool ok = applySelectedToolToCell(x, y);
    lines = snapshot_.events;
    return ok;
}

bool EngineLocalClient::inspectAgent(const std::string& agent_id, std::vector<std::string>& lines) {
    auto* agent = findAgent(agent_id);
    if (!agent) return false;
    lines.clear();
    lines.push_back(agent->name + "：" + agent->current_intent);
    for (const auto& memory : agent->recent_memory) lines.push_back(memory);
    return true;
}

const EngineAgentView* EngineLocalClient::agentAtCell(int x, int y) const {
    for (const auto& agent : snapshot_.agents) {
        if (agent.x == x && agent.y == y) return &agent;
    }
    return nullptr;
}

const EngineAgentView* EngineLocalClient::findAgent(const std::string& agent_id) const {
    for (const auto& agent : snapshot_.agents) {
        if (agent.agent_id == agent_id) return &agent;
    }
    return nullptr;
}

bool EngineLocalClient::bootstrap() {
    pathfinder::client_protocol::ClientBootstrapRequest request;
    request.client_id = client_id_;
    request.session_id = session_id_;
    request.requested_actor_key = actor_key_;
    request.requested_layer_key = layer_key_;
    request.client_protocol_version = 1;
    request.create_if_missing = true;

    auto response = host_.session_gateway.bootstrap(request);
    if (response.is_error()) {
        last_error_ = "bootstrap_failed";
        return false;
    }
    applyBootstrap(response.value());
    last_error_.clear();
    return true;
}

bool EngineLocalClient::refresh() {
    pathfinder::client_protocol::ClientRefreshRequest request;
    request.client_id = client_id_;
    request.session_id = session_id_;
    request.known_projection_version = snapshot_.projection_version;
    request.requested_layer_key = layer_key_;
    request.requested_scopes = {pathfinder::client_protocol::ClientProjectionScope::FullSafeWorld};

    auto response = host_.session_gateway.refresh(request);
    if (response.is_error()) {
        last_error_ = "refresh_failed";
        return false;
    }
    applyRefresh(response.value());
    last_error_.clear();
    return true;
}

bool EngineLocalClient::submitOption(const std::string& option_id) {
    pathfinder::client_protocol::ClientCommandRequest request;
    request.client_id = client_id_;
    request.session_id = session_id_;
    request.client_sequence = ++client_sequence_;
    request.known_projection_version = snapshot_.projection_version;
    request.submit_mode = pathfinder::client_protocol::ClientSubmitMode::OptionId;
    request.option_id = option_id;
    request.selection_context.target.target_kind = pathfinder::world_command::WorldCommandTargetKind::None;

    auto response = host_.command_gateway.handleCommand(request);
    if (response.is_error()) {
        last_error_ = "command_failed";
        return false;
    }
    applyCommand(response.value());
    return response.value().result.result_kind == pathfinder::world_command::WorldCommandResultKind::Succeeded;
}

void EngineLocalClient::applyBootstrap(const pathfinder::client_protocol::ClientBootstrapResponse& response) {
    rebuildSnapshot(response.full_projection);
    snapshot_.available_commands = response.available_commands;
    snapshot_.events.clear();
    for (const auto& event : response.event_feed) snapshot_.events.push_back(eventText(event));
    rebuildTools();
}

void EngineLocalClient::applyRefresh(const pathfinder::client_protocol::ClientRefreshResponse& response) {
    rebuildSnapshot(response.full_projection);
    snapshot_.available_commands = response.available_commands;
    snapshot_.events.clear();
    for (const auto& event : response.event_feed) snapshot_.events.push_back(eventText(event));
    rebuildTools();
}

void EngineLocalClient::applyCommand(const pathfinder::client_protocol::ClientCommandResponse& response) {
    if (response.requires_full_refresh) {
        refresh();
    } else {
        refresh();
    }
    snapshot_.available_commands = response.available_commands.empty() ? snapshot_.available_commands : response.available_commands;
    for (const auto& event : response.event_feed) snapshot_.events.push_back(eventText(event));
    for (const auto& experience : response.experiences) {
        snapshot_.events.push_back("经验：" + experience.command_key + " → " + experience.effect_key);
    }
    if (snapshot_.events.size() > 80) {
        snapshot_.events.erase(snapshot_.events.begin(), snapshot_.events.end() - 80);
    }
    if (response.result.result_kind == pathfinder::world_command::WorldCommandResultKind::Succeeded) {
        last_error_.clear();
    } else if (!response.result.failure_reason_keys.empty()) {
        last_error_ = response.result.failure_reason_keys.front();
    } else {
        last_error_ = "command_not_succeeded";
    }
    rebuildTools();
}

void EngineLocalClient::rebuildSnapshot(const pathfinder::client_protocol::ClientWorldProjection& projection) {
    snapshot_.projection_version = projection.projection_version;
    snapshot_.tick = host_.world_runtime ? host_.world_runtime->currentWorldTick() : snapshot_.tick;
    snapshot_.cells.clear();
    snapshot_.entities.clear();
    snapshot_.agents.clear();

    int min_x = 0;
    int max_x = 0;
    int min_y = 0;
    int max_y = 0;
    bool first = true;

    for (const auto& patch : projection.visible_cells) {
        EngineCellView cell;
        cell.cell_id = patch.cell_id;
        cell.x = intField(patch.fields, "x");
        cell.y = intField(patch.fields, "y");
        cell.terrain_key = patch.fields.count("terrain_key") ? patch.fields.at("terrain_key") : "plain";
        cell.blocks_movement = boolField(patch.fields, "blocks_movement");
        if (auto it = patch.fields.find("entity_ids"); it != patch.fields.end()) cell.entity_ids = splitCsv(it->second);
        snapshot_.cells.push_back(cell);
        if (first) {
            min_x = max_x = cell.x;
            min_y = max_y = cell.y;
            first = false;
        } else {
            min_x = std::min(min_x, cell.x);
            max_x = std::max(max_x, cell.x);
            min_y = std::min(min_y, cell.y);
            max_y = std::max(max_y, cell.y);
        }
    }

    snapshot_.min_x = min_x;
    snapshot_.min_y = min_y;
    snapshot_.width = first ? 0 : (max_x - min_x + 1);
    snapshot_.height = first ? 0 : (max_y - min_y + 1);

    for (const auto& patch : projection.visible_entities) {
        EngineEntityView entity;
        entity.entity_id = patch.entity_id;
        if (auto it = patch.fields.find("entity_key"); it != patch.fields.end()) entity.entity_key = it->second;
        if (auto it = patch.fields.find("display_name_key"); it != patch.fields.end()) entity.display_name = displayNameForKey(entity.entity_key.empty() ? it->second : entity.entity_key);
        if (entity.display_name.empty()) entity.display_name = displayNameForKey(entity.entity_key);
        entity.x = intField(patch.fields, "x");
        entity.y = intField(patch.fields, "y");
        entity.blocks_movement = boolField(patch.fields, "blocks_movement");
        entity.resource_node = boolField(patch.fields, "resource_node");
        entity.remaining_charges = intField(patch.fields, "remaining_charges");
        entity.numeric_states["health"] = doubleField(patch.fields, "numeric_health", 0.0);
        entity.numeric_states["max_health"] = doubleField(patch.fields, "numeric_max_health", 0.0);
        snapshot_.entities.push_back(std::move(entity));
    }

    EngineAgentView player;
    player.agent_id = actor_key_;
    player.name = "探索者";
    player.current_intent = "由正式引擎管线控制";
    for (const auto& entity : snapshot_.entities) {
        if (entity.entity_key == "player") {
            player.x = entity.x;
            player.y = entity.y;
            player.health = entity.numeric_states.count("health") ? entity.numeric_states.at("health") : 10.0;
            player.max_health = entity.numeric_states.count("max_health") ? entity.numeric_states.at("max_health") : 10.0;
            break;
        }
    }

    for (const auto& patch : projection.inventories) {
        const int entry_count = intField(patch.fields, "entry_count");
        for (int i = 0; i < entry_count; ++i) {
            const std::string prefix = "entry_" + std::to_string(i) + "_";
            EngineInventoryItemView item;
            auto key_it = patch.fields.find(prefix + "entity_key");
            if (key_it == patch.fields.end()) continue;
            item.object_key = key_it->second;
            item.display_name = displayNameForKey(item.object_key);
            item.entry_id = patch.fields.count(prefix + "entry_id") ? patch.fields.at(prefix + "entry_id") : "";
            item.entity_id = patch.fields.count(prefix + "entity_id") ? patch.fields.at(prefix + "entity_id") : "";
            item.quantity = intField(patch.fields, prefix + "quantity", 1);
            player.inventory.push_back(std::move(item));
        }
    }

    for (const auto& patch : projection.knowledge) {
        EngineKnowledgeClaimView claim;
        claim.subject_object_key = patch.fields.count("subject_object_key") ? patch.fields.at("subject_object_key") : patch.fields.count("subject") ? patch.fields.at("subject") : "";
        claim.action_key = patch.fields.count("action_key") ? patch.fields.at("action_key") : patch.fields.count("action") ? patch.fields.at("action") : "";
        claim.effect_key = patch.fields.count("effect_key") ? patch.fields.at("effect_key") : patch.fields.count("effect") ? patch.fields.at("effect") : "";
        claim.status = patch.fields.count("status") ? patch.fields.at("status") : "已记录";
        claim.evidence_count = intField(patch.fields, "evidence_count", 1);
        if (!claim.subject_object_key.empty() || !claim.effect_key.empty()) player.knowledge.push_back(std::move(claim));
    }

    snapshot_.agents.push_back(std::move(player));
}

void EngineLocalClient::rebuildTools() {
    tools_.clear();
    for (const auto& option : snapshot_.available_commands) {
        if (!option.enabled) continue;
        const auto kind = option.command_kind;
        if (kind == pathfinder::world_command::WorldCommandKind::Move ||
            kind == pathfinder::world_command::WorldCommandKind::Wait ||
            kind == pathfinder::world_command::WorldCommandKind::Inspect) {
            continue;
        }
        tools_.push_back({ToolKind::CommandOption, option.option_id, option.label_text, optionCategory(option.command_kind)});
    }
    if (selected_tool_index_ >= static_cast<int>(tools_.size())) selected_tool_index_ = -1;
}

std::optional<pathfinder::world_command::WorldCommandOptionDto> EngineLocalClient::findOptionForCell(int x, int y) const {
    for (const auto& option : snapshot_.available_commands) {
        if (!option.enabled) continue;
        if (!option.target.target_coord) continue;
        const auto& coord = *option.target.target_coord;
        if (coord.x == x && coord.y == y && coord.layer_key == layer_key_) {
            if (option.command_kind != pathfinder::world_command::WorldCommandKind::Inspect) return option;
        }
    }
    return std::nullopt;
}

std::optional<pathfinder::world_command::WorldCommandOptionDto> EngineLocalClient::findOptionByKind(pathfinder::world_command::WorldCommandKind kind) const {
    for (const auto& option : snapshot_.available_commands) {
        if (option.enabled && option.command_kind == kind) return option;
    }
    return std::nullopt;
}

} // namespace pf::client
