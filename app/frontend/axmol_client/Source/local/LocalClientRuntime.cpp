#include "local/LocalClientRuntime.h"
#include "pathfinder/client_http/client_server_runtime_factory.h"
#include <algorithm>
#include <filesystem>
#include <map>
#include <cctype>
#include <sstream>

namespace pf::client {

using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;

namespace {

int inventoryIntField(const std::map<std::string, std::string>& fields, const std::string& key, int fallback) {
    auto it = fields.find(key);
    if (it == fields.end()) return fallback;
    try { return std::stoi(it->second); } catch (...) { return fallback; }
}

double inventoryDoubleField(const std::map<std::string, std::string>& fields, const std::string& key, double fallback) {
    auto it = fields.find(key);
    if (it == fields.end()) return fallback;
    try { return std::stod(it->second); } catch (...) { return fallback; }
}

std::string inventoryStringField(const std::map<std::string, std::string>& fields, const std::string& key, const std::string& fallback = "") {
    auto it = fields.find(key);
    if (it == fields.end()) return fallback;
    return it->second;
}

std::vector<LocalInventoryItemView> parseInventoryEntryFields(const InventoryPatchDto& patch) {
    std::vector<LocalInventoryItemView> items;
    const int entry_count = std::max(0, inventoryIntField(patch.fields, "entry_count", 0));
    items.reserve(static_cast<size_t>(entry_count));

    for (int index = 0; index < entry_count; ++index) {
        const std::string prefix = "entry_" + std::to_string(index) + "_";
        LocalInventoryItemView view;
        view.entry_id = inventoryStringField(patch.fields, prefix + "entry_id", "");
        view.entity_id = inventoryStringField(patch.fields, prefix + "entity_id", "");
        view.entity_key = inventoryStringField(patch.fields, prefix + "entity_key", "unknown_item");
        view.stack_key = inventoryStringField(patch.fields, prefix + "stack_key", view.entity_key);
        view.quantity = std::max(1, inventoryIntField(patch.fields, prefix + "quantity", 1));
        const std::string numeric_prefix = prefix + "numeric_";
        for (const auto& [field_key, field_value] : patch.fields) {
            if (field_key.rfind(numeric_prefix, 0) != 0 || field_key == prefix + "numeric_count") continue;
            const auto numeric_key = field_key.substr(numeric_prefix.size());
            view.numeric_states[numeric_key] = inventoryDoubleField(patch.fields, field_key, 0.0);
        }
        items.push_back(std::move(view));
    }
    return items;
}

bool isPlayerInventoryPatch(const InventoryPatchDto& patch) {
    auto owner_it = patch.fields.find("owner_key");
    if (owner_it != patch.fields.end()) return owner_it->second == "player";
    return patch.inventory_id.find("player") != std::string::npos;
}

std::filesystem::path repoRoot() {
#ifdef PATHFINDER_REPO_ROOT
    return std::filesystem::path(PATHFINDER_REPO_ROOT);
#else
    return std::filesystem::current_path();
#endif
}
} // namespace

LocalClientRuntime::LocalClientRuntime() = default;
LocalClientRuntime::~LocalClientRuntime() = default;

bool LocalClientRuntime::bootstrap() {
    try {
        if (!factory_) {
            const auto old_path = std::filesystem::current_path();
            std::filesystem::current_path(repoRoot());
            factory_ = std::make_unique<pathfinder::client_http::ClientServerRuntimeFactory>();
            std::filesystem::current_path(old_path);
        }

        ClientBootstrapRequest request;
        request.client_id = client_id_;
        request.session_id = session_id_;
        request.requested_actor_key = "player";
        request.requested_layer_key = "surface";
        request.create_if_missing = true;
        auto result = factory_->session_gateway.bootstrap(request);
        if (result.is_error()) {
            last_error_ = firstErrorText(result.errors());
            return false;
        }
        applyBootstrap(result.value());
        last_error_.clear();
        return true;
    } catch (const std::exception& ex) {
        last_error_ = ex.what();
        return false;
    }
}

bool LocalClientRuntime::refresh() {
    if (!factory_ && !bootstrap()) return false;
    ClientRefreshRequest request;
    request.client_id = client_id_;
    request.session_id = session_id_;
    request.known_projection_version = snapshot_.projection_version;
    request.requested_layer_key = "surface";
    auto result = factory_->session_gateway.refresh(request);
    if (result.is_error()) {
        last_error_ = firstErrorText(result.errors());
        return false;
    }
    applyRefresh(result.value());
    last_error_.clear();
    return true;
}

bool LocalClientRuntime::reset() {
    if (!factory_ && !bootstrap()) return false;
    snapshot_.knowledge_items.clear();
    ClientResetRequest request;
    request.client_id = client_id_;
    request.session_id = session_id_;
    request.confirmed = true;
    auto result = factory_->session_gateway.reset(request);
    if (result.is_error()) {
        last_error_ = firstErrorText(result.errors());
        return false;
    }
    applyBootstrap(result.value().bootstrap);
    last_error_.clear();
    return true;
}

bool LocalClientRuntime::submitOption(const std::string& option_id) {
    if (!factory_ && !bootstrap()) return false;
    ClientCommandRequest request;
    request.client_id = client_id_;
    request.session_id = session_id_;
    request.client_sequence = ++client_sequence_;
    request.known_projection_version = snapshot_.projection_version;
    request.submit_mode = ClientSubmitMode::OptionId;
    request.option_id = option_id;
    auto result = factory_->command_gateway.handleCommand(request);
    if (result.is_error()) {
        last_error_ = firstErrorText(result.errors());
        return false;
    }
    const auto response = result.value();
    applyCommand(response);
    if (response.requires_full_refresh || response.projection_patch.requires_full_refresh) {
        if (!refresh()) {
            return false;
        }
    }
    last_error_.clear();
    return true;
}

std::optional<LocalCommandOptionView> LocalClientRuntime::findMoveOptionTo(int x, int y) const {
    for (const auto& option : snapshot_.options) {
        if (!option.enabled || option.command_kind != WorldCommandKind::Move) continue;
        if (option.target_x && option.target_y && *option.target_x == x && *option.target_y == y) {
            return option;
        }
    }
    return std::nullopt;
}

std::optional<LocalCommandOptionView> LocalClientRuntime::findInspectOptionFor(int x, int y) const {
    for (const auto& option : snapshot_.options) {
        if (!option.enabled || option.command_kind != WorldCommandKind::Inspect) continue;
        if (option.target_x && option.target_y && *option.target_x == x && *option.target_y == y) {
            return option;
        }
    }
    return std::nullopt;
}

std::optional<LocalCommandOptionView> LocalClientRuntime::findBestOptionForEntityAt(int x, int y) const {
    std::string entity_id;
    for (const auto& entity : snapshot_.entities) {
        if (entity.x == x && entity.y == y && entity.entity_key != "player") {
            entity_id = entity.entity_id;
            break;
        }
    }
    if (entity_id.empty()) return std::nullopt;

    for (const auto kind : {WorldCommandKind::Gather, WorldCommandKind::Chop, WorldCommandKind::Mine, WorldCommandKind::Dig, WorldCommandKind::Pickup, WorldCommandKind::Inspect}) {
        for (const auto& option : snapshot_.options) {
            if (!option.enabled || option.command_kind != kind) continue;
            if (option.target_entity_id == entity_id) return option;
        }
    }
    return std::nullopt;
}

void LocalClientRuntime::applyBootstrap(const ClientBootstrapResponse& response) {
    applyProjection(response.projection_version, response.full_projection, response.available_commands, response.event_feed);
}

void LocalClientRuntime::applyRefresh(const ClientRefreshResponse& response) {
    applyProjection(response.projection_version, response.full_projection, response.available_commands, response.event_feed);
}

void LocalClientRuntime::applyCommand(const ClientCommandResponse& response) {
    snapshot_.projection_version = response.new_projection_version;
    if (!response.requires_full_refresh && !response.projection_patch.requires_full_refresh) {
        applyProjectionPatch(response.projection_patch);
    }
    snapshot_.options.clear();
    for (const auto& opt : response.available_commands) {
        LocalCommandOptionView view;
        view.option_id = opt.option_id;
        view.command_key = opt.command_key;
        view.label_text = translateInlineText(opt.label_text);
        view.command_kind = opt.command_kind;
        view.enabled = opt.enabled;
        if (opt.target.target_coord) {
            view.target_x = opt.target.target_coord->x;
            view.target_y = opt.target.target_coord->y;
        }
        view.target_entity_id = opt.target.target_entity_id;
        view.target_actor_key = opt.target.target_actor_key;
        view.knowledge_key = opt.target.knowledge_key;
        snapshot_.options.push_back(std::move(view));
    }
    for (const auto& event : response.event_feed) {
        snapshot_.events.push_back(event.body_text.empty() ? (event.title_text.empty() ? event.event_kind : event.title_text) : ((event.title_text.empty() ? event.event_kind : event.title_text) + "：" + event.body_text));
    }
    snapshot_.status_text = response.result.failure_reason_keys.empty()
        ? toString(response.result.result_kind)
        : response.result.failure_reason_keys.front();
}

void LocalClientRuntime::applyProjectionPatch(const WorldProjectionPatchDto& patch) {
    if (patch.new_projection_version != 0) {
        snapshot_.projection_version = patch.new_projection_version;
    }

    for (const auto& cell : patch.changed_cells) {
        const auto matches_cell = [&](const LocalCellView& existing) {
            if (!cell.cell_id.empty() && existing.cell_id == cell.cell_id) return true;
            const int x = parseIntField(cell.fields, "x", existing.x);
            const int y = parseIntField(cell.fields, "y", existing.y);
            return existing.x == x && existing.y == y;
        };
        if (cell.op == PatchOp::Remove || cell.op == PatchOp::Clear) {
            snapshot_.cells.erase(std::remove_if(snapshot_.cells.begin(), snapshot_.cells.end(), matches_cell), snapshot_.cells.end());
            continue;
        }

        if (cell.fields.find("x") == cell.fields.end() || cell.fields.find("y") == cell.fields.end()) {
            continue;
        }

        LocalCellView view;
        view.cell_id = cell.cell_id;
        view.x = parseIntField(cell.fields, "x", 0);
        view.y = parseIntField(cell.fields, "y", 0);
        view.layer_key = stringField(cell.fields, "layer_key", "surface");
        view.terrain_key = stringField(cell.fields, "terrain_key", "unknown");
        view.blocks_movement = parseBoolField(cell.fields, "blocks_movement", false);
        view.visibility = stringField(cell.fields, "visibility", "");

        auto it = std::find_if(snapshot_.cells.begin(), snapshot_.cells.end(), matches_cell);
        if (it == snapshot_.cells.end()) {
            snapshot_.cells.push_back(std::move(view));
        } else {
            *it = std::move(view);
        }
    }

    for (const auto& inventory : patch.changed_inventories) {
        if (!isPlayerInventoryPatch(inventory)) continue;
        if (inventory.op == PatchOp::Remove || inventory.op == PatchOp::Clear) {
            snapshot_.inventory_items.clear();
            snapshot_.inventory_capacity_slots = 9;
            continue;
        }
        snapshot_.inventory_capacity_slots = std::max(9, parseIntField(inventory.fields, "capacity_slots", snapshot_.inventory_capacity_slots));
        snapshot_.inventory_items = parseInventoryEntryFields(inventory);
        localizeInventoryItems(snapshot_.inventory_items);
    }

    for (const auto& entity : patch.changed_entities) {
        const auto matches_entity = [&](const LocalEntityView& existing) {
            return existing.entity_id == entity.entity_id;
        };
        if (entity.op == PatchOp::Remove || entity.op == PatchOp::Clear) {
            snapshot_.entities.erase(std::remove_if(snapshot_.entities.begin(), snapshot_.entities.end(), matches_entity), snapshot_.entities.end());
            continue;
        }

        LocalEntityView view;
        view.entity_id = entity.entity_id;
        view.entity_key = stringField(entity.fields, "entity_key", "unknown");
        view.display_name_key = displayNameForEntityKey(view.entity_key, stringField(entity.fields, "display_name_key", view.entity_key));
        view.actor_key = stringField(entity.fields, "actor_key", "");
        view.x = parseIntField(entity.fields, "x", 0);
        view.y = parseIntField(entity.fields, "y", 0);
        view.layer_key = stringField(entity.fields, "layer_key", "surface");
        view.is_resource_node = parseBoolField(entity.fields, "resource_node", false);

        auto it = std::find_if(snapshot_.entities.begin(), snapshot_.entities.end(), matches_entity);
        if (it == snapshot_.entities.end()) {
            snapshot_.entities.push_back(std::move(view));
        } else {
            *it = std::move(view);
        }
    }

    for (const auto& knowledge : patch.changed_knowledge) {
        const auto knowledge_id = stringField(knowledge.fields, "knowledge_id", "");
        if (knowledge_id.empty()) continue;
        const auto matches_knowledge = [&](const LocalKnowledgeView& existing) {
            return existing.knowledge_id == knowledge_id;
        };
        if (knowledge.op == PatchOp::Remove || knowledge.op == PatchOp::Clear) {
            snapshot_.knowledge_items.erase(std::remove_if(snapshot_.knowledge_items.begin(), snapshot_.knowledge_items.end(), matches_knowledge), snapshot_.knowledge_items.end());
            continue;
        }

        LocalKnowledgeView view;
        view.knowledge_id = knowledge_id;
        view.subject_id = stringField(knowledge.fields, "subject_id", "");
        view.action_key = stringField(knowledge.fields, "action_key", "");
        view.effect_key = stringField(knowledge.fields, "effect_key", "");
        view.status = stringField(knowledge.fields, "status", "");
        view.confidence = stringField(knowledge.fields, "confidence", "");
        view.summary_text = translateInlineText(stringField(knowledge.fields, "summary_text", ""));
        view.recipe_text = translateInlineText(stringField(knowledge.fields, "recipe_text", ""));
        view.actor_key = knowledge.actor_key;
        view.subject_name = displayNameForEntityKey(view.subject_id, view.subject_id);
        view.action_name = translateKey("action." + view.action_key + ".name");
        if (view.action_name == "action." + view.action_key + ".name") view.action_name = view.action_key;
        view.effect_name = translateKey("effect." + view.effect_key + ".name");
        if (view.effect_name == "effect." + view.effect_key + ".name") view.effect_name = view.effect_key;

        auto it = std::find_if(snapshot_.knowledge_items.begin(), snapshot_.knowledge_items.end(), matches_knowledge);
        if (it == snapshot_.knowledge_items.end()) {
            snapshot_.knowledge_items.push_back(std::move(view));
        } else {
            *it = std::move(view);
        }
    }
}

void LocalClientRuntime::applyProjection(
    uint64_t projection_version,
    const ClientWorldProjection& projection,
    const std::vector<WorldCommandOptionDto>& options,
    const std::vector<WorldEventDto>& events) {
    snapshot_.projection_version = projection_version == 0 ? projection.projection_version : projection_version;
    snapshot_.cells.clear();
    snapshot_.entities.clear();
    snapshot_.options.clear();
    snapshot_.inventory_items.clear();
    snapshot_.inventory_capacity_slots = 9;

    for (const auto& cell : projection.visible_cells) {
        if (cell.fields.find("x") == cell.fields.end() || cell.fields.find("y") == cell.fields.end()) {
            continue;
        }

        LocalCellView view;
        view.cell_id = cell.cell_id;
        view.x = parseIntField(cell.fields, "x", 0);
        view.y = parseIntField(cell.fields, "y", 0);
        view.layer_key = stringField(cell.fields, "layer_key", "surface");
        view.terrain_key = stringField(cell.fields, "terrain_key", "unknown");
        view.blocks_movement = parseBoolField(cell.fields, "blocks_movement", false);
        view.visibility = stringField(cell.fields, "visibility", "");
        snapshot_.cells.push_back(std::move(view));
    }

    for (const auto& entity : projection.visible_entities) {
        LocalEntityView view;
        view.entity_id = entity.entity_id;
        view.entity_key = stringField(entity.fields, "entity_key", "unknown");
        view.display_name_key = displayNameForEntityKey(view.entity_key, stringField(entity.fields, "display_name_key", view.entity_key));
        view.actor_key = stringField(entity.fields, "actor_key", "");
        view.x = parseIntField(entity.fields, "x", 0);
        view.y = parseIntField(entity.fields, "y", 0);
        view.layer_key = stringField(entity.fields, "layer_key", "surface");
        view.is_resource_node = parseBoolField(entity.fields, "resource_node", false);
        snapshot_.entities.push_back(std::move(view));
    }

    for (const auto& inventory : projection.inventories) {
        if (!isPlayerInventoryPatch(inventory)) continue;
        snapshot_.inventory_capacity_slots = std::max(9, parseIntField(inventory.fields, "capacity_slots", snapshot_.inventory_capacity_slots));
        snapshot_.inventory_items = parseInventoryEntryFields(inventory);
        localizeInventoryItems(snapshot_.inventory_items);
    }

    for (const auto& knowledge : projection.knowledge) {
        const auto knowledge_id = stringField(knowledge.fields, "knowledge_id", "");
        if (knowledge_id.empty()) continue;
        const auto matches_knowledge = [&](const LocalKnowledgeView& existing) {
            return existing.knowledge_id == knowledge_id;
        };
        LocalKnowledgeView view;
        view.knowledge_id = knowledge_id;
        view.subject_id = stringField(knowledge.fields, "subject_id", "");
        view.action_key = stringField(knowledge.fields, "action_key", "");
        view.effect_key = stringField(knowledge.fields, "effect_key", "");
        view.status = stringField(knowledge.fields, "status", "");
        view.confidence = stringField(knowledge.fields, "confidence", "");
        view.summary_text = translateInlineText(stringField(knowledge.fields, "summary_text", ""));
        view.recipe_text = translateInlineText(stringField(knowledge.fields, "recipe_text", ""));
        view.actor_key = knowledge.actor_key;
        view.subject_name = displayNameForEntityKey(view.subject_id, view.subject_id);
        view.action_name = translateKey("action." + view.action_key + ".name");
        if (view.action_name == "action." + view.action_key + ".name") view.action_name = view.action_key;
        view.effect_name = translateKey("effect." + view.effect_key + ".name");
        if (view.effect_name == "effect." + view.effect_key + ".name") view.effect_name = view.effect_key;

        auto it = std::find_if(snapshot_.knowledge_items.begin(), snapshot_.knowledge_items.end(), matches_knowledge);
        if (it == snapshot_.knowledge_items.end()) {
            snapshot_.knowledge_items.push_back(std::move(view));
        } else {
            *it = std::move(view);
        }
    }

    for (const auto& opt : options) {
        LocalCommandOptionView view;
        view.option_id = opt.option_id;
        view.command_key = opt.command_key;
        view.label_text = translateInlineText(opt.label_text);
        view.command_kind = opt.command_kind;
        view.enabled = opt.enabled;
        if (opt.target.target_coord) {
            view.target_x = opt.target.target_coord->x;
            view.target_y = opt.target.target_coord->y;
        }
        view.target_entity_id = opt.target.target_entity_id;
        view.target_actor_key = opt.target.target_actor_key;
        view.knowledge_key = opt.target.knowledge_key;
        snapshot_.options.push_back(std::move(view));
    }

    for (const auto& event : events) {
        snapshot_.events.push_back(event.body_text.empty() ? (event.title_text.empty() ? event.event_kind : event.title_text) : ((event.title_text.empty() ? event.event_kind : event.title_text) + "：" + event.body_text));
    }
    if (snapshot_.events.size() > 8) {
        snapshot_.events.erase(snapshot_.events.begin(), snapshot_.events.end() - 8);
    }

    snapshot_.status_text = "本地 Runtime 已连接";
}


void LocalClientRuntime::localizeInventoryItems(std::vector<LocalInventoryItemView>& items) const {
    for (auto& item : items) {
        item.display_name = displayNameForEntityKey(item.entity_key, item.entity_key.empty() ? item.stack_key : item.entity_key);
    }
}

std::string LocalClientRuntime::translateKey(const std::string& key) const {
    if (!factory_ || !factory_->content_registry || key.empty()) return key;
    const auto translated = factory_->content_registry->translate("zh_cn", key);
    if (translated != key) return translated;

    const std::string entity_prefix = "entity.";
    if (key.rfind(entity_prefix, 0) == 0) {
        const auto entity_key = key.substr(entity_prefix.size());
        const auto object_name = factory_->content_registry->translate("zh_cn", "object." + entity_key + ".name");
        if (object_name != "object." + entity_key + ".name") return object_name;
    }
    return key;
}

std::string LocalClientRuntime::translateInlineText(const std::string& text) const {
    if (text.empty()) return text;
    const auto exact = translateKey(text);
    if (exact != text) return exact;

    std::string result;
    std::string token;
    auto flush = [&]() {
        if (token.empty()) return;
        result += translateKey(token);
        token.clear();
    };
    for (char ch : text) {
        const unsigned char value = static_cast<unsigned char>(ch);
        const bool token_char = std::isalnum(value) || ch == '_' || ch == '.' || ch == ':' || ch == '-';
        if (token_char) {
            token.push_back(ch);
        } else {
            flush();
            result.push_back(ch);
        }
    }
    flush();
    return result;
}

std::string LocalClientRuntime::displayNameForEntityKey(const std::string& entity_key, const std::string& fallback) const {
    const auto translated_fallback = translateKey(fallback);
    if (translated_fallback != fallback) return translated_fallback;

    if (!entity_key.empty()) {
        const auto object_name = translateKey("object." + entity_key + ".name");
        if (object_name != "object." + entity_key + ".name") return object_name;
    }
    return translateInlineText(fallback);
}

std::string LocalClientRuntime::firstErrorText(const std::vector<pathfinder::foundation::ErrorDetail>& errors) {
    if (errors.empty()) return "unknown_error";
    if (errors.front().debug_message) return *errors.front().debug_message;
    return errors.front().message_key;
}

int LocalClientRuntime::parseIntField(const std::map<std::string, std::string>& fields, const std::string& key, int fallback) {
    auto it = fields.find(key);
    if (it == fields.end()) return fallback;
    try { return std::stoi(it->second); } catch (...) { return fallback; }
}

bool LocalClientRuntime::parseBoolField(const std::map<std::string, std::string>& fields, const std::string& key, bool fallback) {
    auto it = fields.find(key);
    if (it == fields.end()) return fallback;
    if (it->second == "true") return true;
    if (it->second == "false") return false;
    return fallback;
}

std::string LocalClientRuntime::stringField(const std::map<std::string, std::string>& fields, const std::string& key, const std::string& fallback) {
    auto it = fields.find(key);
    if (it == fields.end()) return fallback;
    return it->second;
}

} // namespace pf::client
