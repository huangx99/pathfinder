#include "pathfinder/world_inventory/inventory_projection_adapter.h"
#include <sstream>
#include <string>

namespace pathfinder::world_inventory {

using pathfinder::foundation::Result;
using pathfinder::world_command::InventoryPatchDto;
using pathfinder::world_command::PatchOp;

namespace {

// Minimal JSON string escape helper
std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string buildEntriesJson(const std::vector<InventoryItemEntry>& entries) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        if (i > 0) json << ",";
        json << "{";
        json << "\"entry_id\":\"" << jsonEscape(entry.entry_id) << "\",";
        json << "\"entity_id\":\"" << jsonEscape(entry.entity_id) << "\",";
        json << "\"entity_key\":\"" << jsonEscape(entry.entity_key) << "\",";
        json << "\"stack_key\":\"" << jsonEscape(entry.stack_key) << "\",";
        json << "\"quantity\":" << entry.quantity << ",";
        json << "\"numeric_states\":{";
        size_t numeric_index = 0;
        for (const auto& [key, value] : entry.numeric_states) {
            if (numeric_index++ > 0) json << ",";
            json << "\"" << jsonEscape(key) << "\":" << value;
        }
        json << "},";
        json << "\"location_kind\":\"" << toString(InventoryLocationKind::InInventory) << "\"";
        json << "}";
    }
    json << "]";
    return json.str();
}

} // anonymous namespace

Result<std::vector<InventoryPatchDto>> InventoryProjectionAdapter::buildInventoryPatches(
    const std::vector<std::string>& changed_inventory_ids,
    const std::string& viewer_actor_key,
    const IInventoryRuntime& inventory_runtime) const {

    std::vector<InventoryPatchDto> patches;

    for (const auto& inventory_id : changed_inventory_ids) {
        auto inv_res = inventory_runtime.findInventory(inventory_id);
        if (inv_res.is_error()) continue;

        const auto* inv = inv_res.value();

        // Visibility check: viewer can see their own inventory and public inventories
        bool visible = false;
        if (inv->owner.owner_kind == InventoryOwnerKind::Actor && inv->owner.owner_key == viewer_actor_key) {
            visible = true;
        } else if (inv->public_read) {
            visible = true;
        } else if (viewer_actor_key == "player" && inv->owner.owner_kind == InventoryOwnerKind::Actor) {
            visible = true;
        }

        if (!visible) continue;

        InventoryPatchDto patch;
        patch.inventory_id = inventory_id;
        patch.op = PatchOp::Update;
        patch.fields["owner_kind"] = toString(inv->owner.owner_kind);
        patch.fields["owner_key"] = inv->owner.owner_key;
        patch.fields["capacity_slots"] = std::to_string(inv->capacity_slots);
        patch.fields["used_slots"] = std::to_string(inv->used_slots);
        patch.fields["entry_count"] = std::to_string(static_cast<int>(inv->entries.size()));
        for (size_t index = 0; index < inv->entries.size(); ++index) {
            const auto& entry = inv->entries[index];
            const std::string prefix = "entry_" + std::to_string(index) + "_";
            patch.fields[prefix + "entry_id"] = entry.entry_id;
            patch.fields[prefix + "entity_id"] = entry.entity_id;
            patch.fields[prefix + "entity_key"] = entry.entity_key;
            patch.fields[prefix + "stack_key"] = entry.stack_key;
            patch.fields[prefix + "quantity"] = std::to_string(entry.quantity);
            patch.fields[prefix + "numeric_count"] = std::to_string(static_cast<int>(entry.numeric_states.size()));
            for (const auto& [key, value] : entry.numeric_states) {
                patch.fields[prefix + "numeric_" + key] = std::to_string(value);
            }
            patch.fields[prefix + "location_kind"] = toString(InventoryLocationKind::InInventory);
        }
        patch.fields["entries_json"] = buildEntriesJson(inv->entries);

        patches.push_back(std::move(patch));
    }

    return Result<std::vector<InventoryPatchDto>>::ok(std::move(patches));
}

} // namespace pathfinder::world_inventory
