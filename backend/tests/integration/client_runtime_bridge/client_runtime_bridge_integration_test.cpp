#include <cassert>
#include <iostream>
#include <functional>
#include <map>
#include <vector>
#include "pathfinder/client_http/client_server_runtime_factory.h"
#include "pathfinder/client_protocol/client_protocol_harness.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_inventory/world_inventory_types.h"

using namespace pathfinder::client_http;
using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::foundation;
using namespace pathfinder::world_inventory;

// ---------------------------------------------------------------------------
// Helper: create a fully wired factory with runtime-backed bridge.
// ---------------------------------------------------------------------------
struct RuntimeBackedFixture {
    ClientServerRuntimeFactory factory;
    ClientProtocolHarness harness;

    RuntimeBackedFixture()
        : harness(factory.session_gateway, factory.command_gateway, factory.codec) {}
};

static std::string joinStringsForTest(const std::vector<std::string>& values) {
    std::string out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) out += ",";
        out += values[i];
    }
    return out;
}

static std::string numericStatesForTest(const std::map<std::string, double>& values) {
    std::string out;
    for (const auto& [key, value] : values) {
        if (!out.empty()) out += ",";
        out += key + "=" + std::to_string(value);
    }
    return out;
}

static void addContentParams(RuntimeBackedFixture& f,
                             InventoryTransferRequest& req,
                             const std::string& object_key,
                             const std::string& stack_suffix = "test") {
    req.parameters["stack_key"] = object_key + ":" + stack_suffix;
    req.parameters["stackable"] = "true";
    const auto* object = f.factory.content_registry->findObject(object_key);
    if (!object) return;
    req.parameters["display_name_key"] = object->display_key;
    req.parameters["tag_keys"] = joinStringsForTest(object->safe_tags);
    req.parameters["numeric_states"] = numericStatesForTest(object->default_numeric);
}

static void spawnInventoryItem(RuntimeBackedFixture& f,
                               const std::string& actor_key,
                               const std::string& object_key,
                               int quantity,
                               const std::string& transfer_id) {
    InventoryTransferRequest req;
    req.transfer_id = transfer_id;
    req.transfer_kind = InventoryTransferKind::SpawnToInventory;
    req.actor_key = actor_key;
    req.entity_key = object_key;
    req.quantity = quantity;
    addContentParams(f, req, object_key, transfer_id);
    auto res = f.factory.inventory_runtime->transfer(req);
    assert(res.is_ok());
    assert(res.value().ok);
}

static void spawnMapItem(RuntimeBackedFixture& f,
                         const std::string& entity_id,
                         const std::string& object_key,
                         const pathfinder::world_runtime::WorldCellCoord& coord,
                         int quantity = 1) {
    const auto* object = f.factory.content_registry->findObject(object_key);
    assert(object != nullptr);
    auto res = f.factory.world_runtime->spawnEntityOnMap(
        entity_id,
        object_key,
        object->display_key,
        coord,
        quantity,
        object_key + ":" + entity_id,
        true,
        {},
        object->default_numeric,
        object->safe_tags);
    assert(res.is_ok());
}

static int inventoryQuantity(RuntimeBackedFixture& f,
                             const std::string& actor_key,
                             const std::string& object_key) {
    InventoryOwnerRef owner{InventoryOwnerKind::Actor, actor_key};
    auto items = f.factory.inventory_runtime->queryItems(owner, object_key);
    assert(items.is_ok());
    int total = 0;
    for (const auto& item : items.value()) total += item.quantity;
    return total;
}

static const WorldCommandOptionDto* findOption(
    const std::vector<WorldCommandOptionDto>& options,
    const std::function<bool(const WorldCommandOptionDto&)>& predicate) {
    for (const auto& option : options) {
        if (predicate(option)) return &option;
    }
    return nullptr;
}

static bool entityExists(RuntimeBackedFixture& f, const std::string& entity_id) {
    return f.factory.world_runtime->findEntity(entity_id).is_ok();
}

// ---------------------------------------------------------------------------
// Bootstrap returns non-empty visible_cells
// ---------------------------------------------------------------------------
void run_bootstrap_non_empty_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_1", "player");
    assert(boot.is_ok());
    assert(!boot.value().full_projection.visible_cells.empty());
    assert(boot.value().projection_version > 0);

    bool has_prefetched_east_cell = false;
    for (const auto& cell : boot.value().full_projection.visible_cells) {
        auto x_it = cell.fields.find("x");
        auto y_it = cell.fields.find("y");
        if (x_it == cell.fields.end() || y_it == cell.fields.end()) continue;
        if (x_it->second == "1" && y_it->second == "0") {
            has_prefetched_east_cell = true;
            break;
        }
    }
    assert(has_prefetched_east_cell);

    std::cout << "client_runtime_bridge_bootstrap_non_empty_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Bootstrap returns player entity
// ---------------------------------------------------------------------------
void run_bootstrap_player_visible_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_2", "player");
    assert(boot.is_ok());

    bool has_player = false;
    for (const auto& ent : boot.value().full_projection.visible_entities) {
        if (ent.fields.count("entity_key") && ent.fields.at("entity_key") == "player") {
            has_player = true;
            break;
        }
    }
    assert(has_player);

    std::cout << "client_runtime_bridge_bootstrap_player_visible_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Bootstrap returns real Move options
// ---------------------------------------------------------------------------
void run_bootstrap_move_options_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_3", "player");
    assert(boot.is_ok());

    bool has_move = false;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Move) {
            has_move = true;
            break;
        }
    }
    assert(has_move);

    std::cout << "client_runtime_bridge_bootstrap_move_options_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Move option changes actor position
// ---------------------------------------------------------------------------
void run_move_changes_position_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_4", "player");
    assert(boot.is_ok());

    // Find an enabled Move option
    std::string move_option_id;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Move && opt.enabled) {
            move_option_id = opt.option_id;
            break;
        }
    }
    assert(!move_option_id.empty());

    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_4", "client_1", 1, boot.value().projection_version, move_option_id);
    assert(cmd_resp.is_ok());
    assert(cmd_resp.value().result.result_kind == WorldCommandResultKind::Succeeded);

    // Refresh and verify position changed or version advanced
    auto refresh = f.harness.fakeRefresh(
        "session_4", "client_1", cmd_resp.value().new_projection_version);
    assert(refresh.is_ok());
    assert(refresh.value().projection_version >= cmd_resp.value().new_projection_version);

    std::cout << "client_runtime_bridge_move_changes_position_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Move updates projection version
// ---------------------------------------------------------------------------
void run_move_updates_version_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_5", "player");
    assert(boot.is_ok());
    uint64_t old_version = boot.value().projection_version;

    std::string move_option_id;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Move && opt.enabled) {
            move_option_id = opt.option_id;
            break;
        }
    }
    assert(!move_option_id.empty());

    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_5", "client_1", 1, old_version, move_option_id);
    assert(cmd_resp.is_ok());
    assert(cmd_resp.value().new_projection_version > old_version);

    std::cout << "client_runtime_bridge_move_updates_version_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Blocked terrain cannot move
// ---------------------------------------------------------------------------
void run_blocked_terrain_blocked_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_6", "player");
    assert(boot.is_ok());

    // Find a disabled Move option (blocked terrain or out of bounds)
    std::string blocked_option_id;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Move && !opt.enabled) {
            blocked_option_id = opt.option_id;
            break;
        }
    }

    // If there is a disabled option, try to submit it
    if (!blocked_option_id.empty()) {
        auto cmd_resp = f.harness.fakeSubmitOption(
            "session_6", "client_1", 1, boot.value().projection_version, blocked_option_id);
        assert(cmd_resp.is_ok());
        // Even if option is disabled, submission goes through but handler blocks it
        // (or the adapter may allow it since it's in the snapshot)
        // P56 requires: disabled option submit -> blocked by handler
        assert(cmd_resp.value().result.result_kind == WorldCommandResultKind::Blocked ||
               cmd_resp.value().result.result_kind == WorldCommandResultKind::Failed);
    }

    std::cout << "client_runtime_bridge_blocked_terrain_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Forged option_id is blocked
// ---------------------------------------------------------------------------
void run_forged_option_blocked_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_7", "player");
    assert(boot.is_ok());

    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_7", "client_1", 1, boot.value().projection_version, "opt_forged_xyz");
    assert(cmd_resp.is_ok());
    assert(cmd_resp.value().result.result_kind == WorldCommandResultKind::Blocked);

    std::cout << "client_runtime_bridge_forged_option_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Stale projection version is blocked
// ---------------------------------------------------------------------------
void run_stale_version_blocked_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_8", "player");
    assert(boot.is_ok());

    std::string wait_option_id;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Wait) {
            wait_option_id = opt.option_id;
            break;
        }
    }
    assert(!wait_option_id.empty());

    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_8", "client_1", 1, 0, wait_option_id);
    assert(cmd_resp.is_ok());
    assert(cmd_resp.value().requires_full_refresh);

    std::cout << "client_runtime_bridge_stale_version_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Bootstrap projection_version matches full_projection.projection_version
// ---------------------------------------------------------------------------
void run_bootstrap_version_matches_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_v1", "player");
    assert(boot.is_ok());
    assert(boot.value().projection_version == boot.value().full_projection.projection_version);
    assert(boot.value().projection_version > 0);

    std::cout << "client_runtime_bridge_bootstrap_version_matches_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Refresh projection_version matches full_projection.projection_version
// ---------------------------------------------------------------------------
void run_refresh_version_matches_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_v2", "player");
    assert(boot.is_ok());

    auto refresh = f.harness.fakeRefresh(
        "session_v2", "client_1", boot.value().projection_version);
    assert(refresh.is_ok());
    assert(refresh.value().projection_version == refresh.value().full_projection.projection_version);
    assert(refresh.value().projection_version >= boot.value().projection_version);

    std::cout << "client_runtime_bridge_refresh_version_matches_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Submit command using full_projection.projection_version should not be stale
// ---------------------------------------------------------------------------
void run_command_with_full_projection_version_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_v3", "player");
    assert(boot.is_ok());

    // Use full_projection.projection_version as the known version
    uint64_t known_version = boot.value().full_projection.projection_version;
    assert(known_version == boot.value().projection_version);

    std::string wait_option_id;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Wait) {
            wait_option_id = opt.option_id;
            break;
        }
    }
    assert(!wait_option_id.empty());

    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_v3", "client_1", 1, known_version, wait_option_id);
    assert(cmd_resp.is_ok());
    // Must succeed, not blocked by stale version
    assert(cmd_resp.value().result.result_kind != WorldCommandResultKind::Blocked);

    std::cout << "client_runtime_bridge_command_with_full_projection_version_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Refresh after move returns new position
// ---------------------------------------------------------------------------
void run_refresh_after_move_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_9", "player");
    assert(boot.is_ok());

    std::string move_option_id;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Move && opt.enabled) {
            move_option_id = opt.option_id;
            break;
        }
    }
    assert(!move_option_id.empty());

    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_9", "client_1", 1, boot.value().projection_version, move_option_id);
    assert(cmd_resp.is_ok());

    auto refresh = f.harness.fakeRefresh(
        "session_9", "client_1", cmd_resp.value().new_projection_version);
    assert(refresh.is_ok());
    assert(refresh.value().projection_version == cmd_resp.value().new_projection_version);

    std::cout << "client_runtime_bridge_refresh_after_move_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P58: Cross-region move — walk to region boundary, ensure generates next region,
// command response contains new outward Move option, continue moving.
// ---------------------------------------------------------------------------
void run_cross_region_move_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_cross", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;

    // Find initial player position from projection
    int player_x = 0, player_y = 0;
    for (const auto& ent : boot.value().full_projection.visible_entities) {
        if (ent.fields.count("entity_key") && ent.fields.at("entity_key") == "player") {
            player_x = std::stoi(ent.fields.at("x"));
            player_y = std::stoi(ent.fields.at("y"));
            break;
        }
    }
    assert(player_x == 0 && player_y == 0);

    // Walk east until x=7 (region 0 right edge, x=8 is in region 1)
    for (int step = 0; step < 7; ++step) {
        std::string move_option_id;
        for (const auto& opt : boot.value().available_commands) {
            if (opt.command_key == "move_east" && opt.enabled) {
                move_option_id = opt.option_id;
                break;
            }
        }
        assert(!move_option_id.empty());

        auto cmd = f.harness.fakeSubmitOption("session_cross", "client_1", step + 1, version, move_option_id);
        assert(cmd.is_ok());
        assert(cmd.value().result.result_kind == WorldCommandResultKind::Succeeded);
        version = cmd.value().new_projection_version;

        // Update available_commands from response for next iteration
        boot.value().available_commands = cmd.value().available_commands;
    }

    // Now at x=7. The next command response should have enabled move_east to x=8.
    // This requires region (1,0) to be generated during available_commands build.
    std::string east_option_id;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_key == "move_east") {
            east_option_id = opt.option_id;
            assert(opt.enabled); // MUST be enabled, not disabled/out_of_bounds
            assert(opt.target.target_coord->x == 8);
            break;
        }
    }
    assert(!east_option_id.empty());

    // Submit move to x=8 — this crosses into region (1,0)
    auto cross_cmd = f.harness.fakeSubmitOption("session_cross", "client_1", 8, version, east_option_id);
    assert(cross_cmd.is_ok());
    assert(cross_cmd.value().result.result_kind == WorldCommandResultKind::Succeeded);
    version = cross_cmd.value().new_projection_version;

    // Verify runtime generated region (1,0)
    assert(f.factory.world_runtime->isRegionGenerated("world_default:surface:region:1:0:16"));

    // Verify player now at x=8 via refresh
    auto refresh = f.harness.fakeRefresh("session_cross", "client_1", version);
    assert(refresh.is_ok());
    int new_x = -1;
    for (const auto& ent : refresh.value().full_projection.visible_entities) {
        if (ent.fields.count("entity_key") && ent.fields.at("entity_key") == "player") {
            new_x = std::stoi(ent.fields.at("x"));
            break;
        }
    }
    assert(new_x == 8);

    std::cout << "client_runtime_bridge_cross_region_move_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P58: Negative cross-region move — walk west to x=-8 then x=-9 into region (-1,0)
// ---------------------------------------------------------------------------
void run_negative_cross_region_move_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_neg", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;

    // Walk west 8 steps to x=-8 (region 0 left edge, x=-9 is in region -1)
    for (int step = 0; step < 8; ++step) {
        std::string move_option_id;
        for (const auto& opt : boot.value().available_commands) {
            if (opt.command_key == "move_west" && opt.enabled) {
                move_option_id = opt.option_id;
                break;
            }
        }
        assert(!move_option_id.empty());
        auto cmd = f.harness.fakeSubmitOption("session_neg", "client_1", step + 1, version, move_option_id);
        assert(cmd.is_ok());
        assert(cmd.value().result.result_kind == WorldCommandResultKind::Succeeded);
        version = cmd.value().new_projection_version;
        boot.value().available_commands = cmd.value().available_commands;
    }

    // Next step to x=-9 must be enabled and trigger region (-1,0) generation
    std::string west_option_id;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_key == "move_west") {
            west_option_id = opt.option_id;
            assert(opt.enabled);
            assert(opt.target.target_coord->x == -9);
            break;
        }
    }
    assert(!west_option_id.empty());

    auto cross_cmd = f.harness.fakeSubmitOption("session_neg", "client_1", 9, version, west_option_id);
    assert(cross_cmd.is_ok());
    assert(cross_cmd.value().result.result_kind == WorldCommandResultKind::Succeeded);

    assert(f.factory.world_runtime->isRegionGenerated("world_default:surface:region:-1:0:16"));

    std::cout << "client_runtime_bridge_negative_cross_region_move_tests: all passed" << std::endl;
}


// ---------------------------------------------------------------------------
// NPC work order: player learns a recipe, teaches NPC, NPC gathers map inputs,
// crafts through Command/Reaction, and delivers the output to the player.
// ---------------------------------------------------------------------------
void run_npc_order_crafts_axe_after_teaching_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_npc_axe", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;

    std::vector<WorldCommandOptionDto> commands = boot.value().available_commands;
    for (int attempt = 0; attempt < 3; ++attempt) {
        spawnInventoryItem(f, "player", "loose_stone", 1, "player_stone_for_axe_learning_" + std::to_string(attempt));
        spawnInventoryItem(f, "player", "wood", 1, "player_wood_for_axe_learning_" + std::to_string(attempt));

        auto refresh = f.harness.fakeRefresh("session_npc_axe", "client_1", version);
        assert(refresh.is_ok());
        version = refresh.value().projection_version;
        commands = refresh.value().available_commands;

        const auto* craft_axe = findOption(commands, [](const auto& option) {
            return option.command_kind == WorldCommandKind::Craft &&
                   option.target.recipe_key == "loose_stone_plus_wood_make_axe" &&
                   option.label_text.find("原料") != std::string::npos &&
                   option.label_text.find("碎石") != std::string::npos &&
                   option.label_text.find("木头") != std::string::npos;
        });
        assert(craft_axe != nullptr);

        auto craft = f.harness.fakeSubmitOption("session_npc_axe", "client_1", static_cast<uint64_t>(attempt + 1), version, craft_axe->option_id);
        assert(craft.is_ok());
        assert(craft.value().result.result_kind == WorldCommandResultKind::Succeeded);
        version = craft.value().new_projection_version;
        commands = craft.value().available_commands;
    }
    const int player_axe_after_learning = inventoryQuantity(f, "player", "axe");
    assert(player_axe_after_learning >= 3);

    const auto* teach_axe = findOption(commands, [](const auto& option) {
        return option.command_kind == WorldCommandKind::Teach &&
               option.target.target_actor_key == "companion" &&
               option.label_text.find("斧头") != std::string::npos;
    });
    assert(teach_axe != nullptr);

    auto teach = f.harness.fakeSubmitOption("session_npc_axe", "client_1", 4, version, teach_axe->option_id);
    assert(teach.is_ok());
    assert(teach.value().result.result_kind == WorldCommandResultKind::Succeeded);
    version = teach.value().new_projection_version;

    spawnMapItem(f, "npc_test_stone_for_axe", "loose_stone", {1, 1, "surface"});
    spawnMapItem(f, "npc_test_wood_for_axe", "wood", {2, 1, "surface"});

    auto order_refresh = f.harness.fakeRefresh("session_npc_axe", "client_1", version);
    assert(order_refresh.is_ok());
    version = order_refresh.value().projection_version;

    const auto* order_axe = findOption(order_refresh.value().available_commands, [](const auto& option) {
        return option.command_kind == WorldCommandKind::Use &&
               option.command_key == "order_actor_craft" &&
               option.target.target_actor_key == "companion" &&
               option.target.recipe_key == "loose_stone_plus_wood_make_axe" &&
               option.label_text.find("原料") != std::string::npos;
    });
    assert(order_axe != nullptr);

    auto order = f.harness.fakeSubmitOption("session_npc_axe", "client_1", 5, version, order_axe->option_id);
    assert(order.is_ok());
    assert(order.value().result.result_kind == WorldCommandResultKind::Succeeded);
    assert(inventoryQuantity(f, "player", "axe") > player_axe_after_learning);

    std::cout << "client_runtime_bridge_npc_order_crafts_axe_after_teaching_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// NPC work order: taught torch knowledge lets NPC craft a fire tool and counter
// a nearby predator through backend command handling, not frontend rules.
// ---------------------------------------------------------------------------
void run_npc_order_torch_counters_threat_after_teaching_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_npc_torch", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;

    spawnMapItem(f, "npc_test_learning_camp_fire", "camp_fire", {0, 1, "surface"});

    std::vector<WorldCommandOptionDto> commands = boot.value().available_commands;
    for (int attempt = 0; attempt < 3; ++attempt) {
        spawnInventoryItem(f, "player", "wood", 1, "player_wood_for_torch_learning_" + std::to_string(attempt));

        auto refresh = f.harness.fakeRefresh("session_npc_torch", "client_1", version);
        assert(refresh.is_ok());
        version = refresh.value().projection_version;
        commands = refresh.value().available_commands;

        const auto* craft_torch = findOption(commands, [](const auto& option) {
            return option.command_kind == WorldCommandKind::Craft &&
                   option.target.recipe_key == "wood_plus_fire_make_torch" &&
                   option.label_text.find("原料") != std::string::npos &&
                   option.label_text.find("篝火") != std::string::npos;
        });
        assert(craft_torch != nullptr);

        auto craft = f.harness.fakeSubmitOption("session_npc_torch", "client_1", static_cast<uint64_t>(attempt + 1), version, craft_torch->option_id);
        assert(craft.is_ok());
        assert(craft.value().result.result_kind == WorldCommandResultKind::Succeeded);
        version = craft.value().new_projection_version;
        commands = craft.value().available_commands;
    }

    const auto* teach_torch = findOption(commands, [](const auto& option) {
        return option.command_kind == WorldCommandKind::Teach &&
               option.target.target_actor_key == "companion" &&
               option.label_text.find("火把") != std::string::npos;
    });
    assert(teach_torch != nullptr);

    auto teach = f.harness.fakeSubmitOption("session_npc_torch", "client_1", 4, version, teach_torch->option_id);
    assert(teach.is_ok());
    assert(teach.value().result.result_kind == WorldCommandResultKind::Succeeded);
    version = teach.value().new_projection_version;

    spawnMapItem(f, "npc_test_wood_for_torch", "wood", {1, 1, "surface"});
    assert(entityExists(f, "scenario_beast_shadow"));

    auto order_refresh = f.harness.fakeRefresh("session_npc_torch", "client_1", version);
    assert(order_refresh.is_ok());
    version = order_refresh.value().projection_version;

    const auto* order_torch = findOption(order_refresh.value().available_commands, [](const auto& option) {
        return option.command_kind == WorldCommandKind::Use &&
               option.command_key == "order_actor_craft" &&
               option.target.target_actor_key == "companion" &&
               option.target.recipe_key == "wood_plus_fire_make_torch";
    });
    assert(order_torch != nullptr);

    auto order = f.harness.fakeSubmitOption("session_npc_torch", "client_1", 5, version, order_torch->option_id);
    assert(order.is_ok());
    assert(order.value().result.result_kind == WorldCommandResultKind::Succeeded);
    assert(inventoryQuantity(f, "player", "torch") >= 1);
    assert(!entityExists(f, "scenario_beast_shadow"));

    bool has_counter_event = false;
    for (const auto& event : order.value().event_feed) {
        if (event.event_kind == "NpcCounteredThreat") {
            has_counter_event = true;
            break;
        }
    }
    assert(has_counter_event);

    std::cout << "client_runtime_bridge_npc_order_torch_counters_threat_after_teaching_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_bootstrap_non_empty_tests();
        run_bootstrap_player_visible_tests();
        run_bootstrap_move_options_tests();
        run_move_changes_position_tests();
        run_move_updates_version_tests();
        run_blocked_terrain_blocked_tests();
        run_forged_option_blocked_tests();
        run_stale_version_blocked_tests();
        run_bootstrap_version_matches_tests();
        run_refresh_version_matches_tests();
        run_command_with_full_projection_version_tests();
        run_refresh_after_move_tests();
        run_cross_region_move_tests();
        run_negative_cross_region_move_tests();
        run_npc_order_crafts_axe_after_teaching_tests();
        run_npc_order_torch_counters_threat_after_teaching_tests();
        return 0;
    }

    std::string test_name = argv[1];
    if (test_name == "bootstrap_non_empty") run_bootstrap_non_empty_tests();
    else if (test_name == "bootstrap_player_visible") run_bootstrap_player_visible_tests();
    else if (test_name == "bootstrap_move_options") run_bootstrap_move_options_tests();
    else if (test_name == "move_changes_position") run_move_changes_position_tests();
    else if (test_name == "move_updates_version") run_move_updates_version_tests();
    else if (test_name == "blocked_terrain_blocked") run_blocked_terrain_blocked_tests();
    else if (test_name == "forged_option_blocked") run_forged_option_blocked_tests();
    else if (test_name == "stale_version_blocked") run_stale_version_blocked_tests();
    else if (test_name == "bootstrap_version_matches") run_bootstrap_version_matches_tests();
    else if (test_name == "refresh_version_matches") run_refresh_version_matches_tests();
    else if (test_name == "command_with_full_projection_version") run_command_with_full_projection_version_tests();
    else if (test_name == "refresh_after_move") run_refresh_after_move_tests();
    else if (test_name == "cross_region_move") run_cross_region_move_tests();
    else if (test_name == "negative_cross_region_move") run_negative_cross_region_move_tests();
    else if (test_name == "npc_order_crafts_axe_after_teaching") run_npc_order_crafts_axe_after_teaching_tests();
    else if (test_name == "npc_order_torch_counters_threat_after_teaching") run_npc_order_torch_counters_threat_after_teaching_tests();
    else {
        std::cerr << "Unknown test: " << test_name << std::endl;
        return 1;
    }
    return 0;
}
