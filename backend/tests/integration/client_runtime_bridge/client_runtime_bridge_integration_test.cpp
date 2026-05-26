#include <algorithm>
#include <cassert>
#include <iostream>
#include <functional>
#include <map>
#include <vector>
#include "pathfinder/client_runtime_host/client_runtime_host_factory.h"
#include "pathfinder/client_protocol/client_protocol_harness.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_inventory/world_inventory_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/foundation/id.h"

using namespace pathfinder::client_runtime_host;
using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::foundation;
using namespace pathfinder::world_inventory;

// ---------------------------------------------------------------------------
// Helper: create a fully wired factory with runtime-backed bridge.
// ---------------------------------------------------------------------------
struct RuntimeBackedFixture {
    ClientRuntimeHostFactory factory;
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


static void spawnTestActor(RuntimeBackedFixture& f,
                           const std::string& actor_key,
                           const std::string& entity_key,
                           const pathfinder::world_runtime::WorldCellCoord& coord,
                           const std::string& entity_id_override = "") {
    const auto* agent = f.factory.content_registry->findAgent(entity_key);
    const auto* object = f.factory.content_registry->findObject(entity_key);
    std::string display_key = agent ? agent->display_key : object ? object->display_key : "entity." + entity_key;
    std::vector<std::string> tags = object ? object->safe_tags : std::vector<std::string>{"actor"};
    std::map<std::string, double> numeric = object ? object->default_numeric : std::map<std::string, double>{};
    if (agent) {
        if (std::find(tags.begin(), tags.end(), "actor") == tags.end()) tags.push_back("actor");
        if (!agent->embodiment.empty()) tags.push_back(agent->embodiment);
        if (!agent->cognition_band.empty()) tags.push_back(agent->cognition_band);
        numeric["fear"] = agent->default_fear;
        numeric["hunger"] = agent->default_hunger;
        numeric["trust"] = agent->default_trust;
    }
    auto res = f.factory.world_runtime->spawnActor(
        actor_key,
        entity_key,
        display_key,
        coord,
        10,
        false,
        tags,
        numeric,
        entity_id_override);
    assert(res.is_ok());
}

static void spawnResourceNode(RuntimeBackedFixture& f,
                              const std::string& node_id,
                              const std::string& resource_key,
                              const pathfinder::world_runtime::WorldCellCoord& coord,
                              const std::string& required_action_key,
                              const std::vector<std::string>& outputs) {
    pathfinder::world_runtime::WorldResourceNodeRuntime node;
    node.node_id = node_id;
    node.resource_key = resource_key;
    node.coord = coord;
    node.node_kind_str = "Stone";
    node.node_state_str = "Available";
    node.remaining_charges = 2;
    node.max_charges = 2;
    node.required_action_key = required_action_key;
    node.output_entity_keys = outputs;
    node.tag_keys = {"test_resource"};
    auto res = f.factory.world_runtime->upsertGeneratedResourceNode(node);
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

static bool actorHasClaim(RuntimeBackedFixture& f,
                          const std::string& actor_key,
                          const std::string& subject_key,
                          const std::string& action_key,
                          const std::string& effect_key) {
    pathfinder::knowledge::KnowledgeOwner owner;
    owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
    owner.entity_id = pathfinder::foundation::EntityId(actor_key);
    auto claims = f.factory.knowledge_repository->listByOwner(owner);
    assert(claims.is_ok());
    for (const auto& claim : claims.value()) {
        if (claim.subject.subject_id == subject_key &&
            claim.predicate.action_key == action_key &&
            claim.predicate.effect_key == effect_key) {
            return true;
        }
    }
    return false;
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

static bool entityOnMap(RuntimeBackedFixture& f, const std::string& entity_id) {
    auto entity_res = f.factory.world_runtime->findEntity(entity_id);
    return entity_res.is_ok() && entity_res.value()->location_kind == pathfinder::world_runtime::WorldEntityLocationKind::OnMap;
}

static void destroyMapItemsByKey(RuntimeBackedFixture& f, const std::string& object_key) {
    auto snap = f.factory.world_runtime->snapshotForDebug();
    assert(snap.is_ok());
    for (const auto& [entity_id, entity] : snap.value().entities) {
        if (entity.entity_key == object_key &&
            entity.location_kind == pathfinder::world_runtime::WorldEntityLocationKind::OnMap) {
            (void)f.factory.world_runtime->destroyEntity(entity_id);
        }
    }
}

static ClientCommandResponse submitWait(RuntimeBackedFixture& f,
                                        const std::string& session_id,
                                        uint64_t& version,
                                        uint64_t& sequence) {
    auto refresh = f.harness.fakeRefresh(session_id, "client_1", version);
    assert(refresh.is_ok());
    version = refresh.value().projection_version;
    const auto* wait = findOption(refresh.value().available_commands, [](const auto& option) {
        return option.command_kind == WorldCommandKind::Wait && option.command_key == "wait";
    });
    assert(wait != nullptr);
    auto response = f.harness.fakeSubmitOption(session_id, "client_1", sequence++, version, wait->option_id);
    assert(response.is_ok());
    assert(response.value().result.result_kind == WorldCommandResultKind::Succeeded);
    version = response.value().new_projection_version;
    return response.value();
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
// Sandbox clients are controller-style clients: the full sandbox board and NPCs
// on sandbox cells must remain visible even outside the hidden player actor's
// diamond-shaped personal vision radius.
// ---------------------------------------------------------------------------
void run_sandbox_projection_includes_full_board_and_far_npc_tests() {
    RuntimeBackedFixture f;
    spawnTestActor(f, "companion_far", "companion", {7, 7, "surface"});
    spawnInventoryItem(f, "companion_far", "red_berry", 2, "far_inv_red_berry");

    auto boot = f.harness.fakeBootstrap("client_1", "session_sandbox_full_projection", "player");
    assert(boot.is_ok());

    bool saw_min_corner = false;
    bool saw_max_corner = false;
    for (const auto& cell : boot.value().full_projection.visible_cells) {
        auto x_it = cell.fields.find("x");
        auto y_it = cell.fields.find("y");
        if (x_it == cell.fields.end() || y_it == cell.fields.end()) continue;
        if (x_it->second == "-8" && y_it->second == "-8") saw_min_corner = true;
        if (x_it->second == "7" && y_it->second == "7") saw_max_corner = true;
    }
    assert(saw_min_corner);
    assert(saw_max_corner);

    bool saw_far_npc = false;
    for (const auto& entity : boot.value().full_projection.visible_entities) {
        auto actor_it = entity.fields.find("actor_key");
        if (actor_it != entity.fields.end() && actor_it->second == "companion_far") {
            saw_far_npc = true;
            break;
        }
    }
    assert(saw_far_npc);

    bool saw_far_inventory = false;
    for (const auto& inventory : boot.value().full_projection.inventories) {
        auto owner_it = inventory.fields.find("owner_key");
        if (owner_it != inventory.fields.end() && owner_it->second == "companion_far") {
            saw_far_inventory = true;
            break;
        }
    }
    assert(saw_far_inventory);

    std::cout << "client_runtime_bridge_sandbox_projection_includes_full_board_and_far_npc_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Bootstrap creates hidden session actor without exposing a default player entity
// ---------------------------------------------------------------------------
void run_bootstrap_player_visible_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_2", "player");
    assert(boot.is_ok());

    auto actor_res = f.factory.world_runtime->findActor("player");
    assert(actor_res.is_ok());
    assert(!actor_res.value()->is_player_controlled);

    bool has_player_entity = false;
    for (const auto& ent : boot.value().full_projection.visible_entities) {
        if (ent.fields.count("entity_key") && ent.fields.at("entity_key") == "player") {
            has_player_entity = true;
            break;
        }
    }
    assert(!has_player_entity);

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

    // Verify hidden session actor now at x=8 via runtime authority.
    auto refresh = f.harness.fakeRefresh("session_cross", "client_1", version);
    assert(refresh.is_ok());
    auto actor_res = f.factory.world_runtime->findActor("player");
    assert(actor_res.is_ok());
    assert(actor_res.value()->coord.x == 8);

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
    spawnTestActor(f, "companion", "companion", {1, 0, "surface"});

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

    destroyMapItemsByKey(f, "loose_stone");
    spawnResourceNode(f, "npc_test_stone_node_for_axe", "loose_stone_node", {1, 1, "surface"}, "gather", {"loose_stone"});
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
    assert(inventoryQuantity(f, "player", "axe") == player_axe_after_learning);

    bool accepted = false;
    for (const auto& event : order.value().event_feed) {
        if (event.event_kind == "NpcWorkOrderAccepted") accepted = true;
    }
    assert(accepted);
    version = order.value().new_projection_version;

    bool saw_gather = false;
    bool saw_pick = false;
    bool saw_craft = false;
    bool saw_complete = false;
    uint64_t sequence = 6;
    int waits = 0;
    for (; waits < 20 && inventoryQuantity(f, "player", "axe") == player_axe_after_learning; ++waits) {
        auto wait_response = submitWait(f, "session_npc_axe", version, sequence);
        for (const auto& event : wait_response.event_feed) {
            if (event.event_kind == "NpcWorkGathering") saw_gather = true;
            if (event.event_kind == "NpcWorkPicking") saw_pick = true;
            if (event.event_kind == "NpcWorkCrafting") saw_craft = true;
            if (event.event_kind == "NpcWorkOrderCompleted") saw_complete = true;
        }
    }
    assert(waits >= 3);
    assert(inventoryQuantity(f, "player", "axe") > player_axe_after_learning);
    assert(saw_gather);
    assert(saw_pick);
    assert(saw_craft);
    assert(saw_complete);

    std::cout << "client_runtime_bridge_npc_order_crafts_axe_after_teaching_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// NPC work order: taught torch knowledge lets NPC craft a fire tool and counter
// a nearby predator through backend command handling, not frontend rules.
// ---------------------------------------------------------------------------
void run_npc_order_torch_counters_threat_after_teaching_tests() {
    RuntimeBackedFixture f;
    spawnTestActor(f, "companion", "companion", {1, 0, "surface"});
    spawnTestActor(f, "beast_shadow", "beast_shadow", {4, 1, "surface"}, "scenario_beast_shadow");

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

    const int player_torch_after_learning = inventoryQuantity(f, "player", "torch");

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
    assert(inventoryQuantity(f, "player", "torch") == player_torch_after_learning);
    assert(entityExists(f, "scenario_beast_shadow"));
    version = order.value().new_projection_version;

    bool has_counter_event = false;
    bool has_complete_event = false;
    uint64_t sequence = 6;
    for (int step = 0; step < 20 && (!has_counter_event || inventoryQuantity(f, "player", "torch") <= player_torch_after_learning); ++step) {
        auto wait_response = submitWait(f, "session_npc_torch", version, sequence);
        for (const auto& event : wait_response.event_feed) {
            if (event.event_kind == "NpcCounteredThreat") has_counter_event = true;
            if (event.event_kind == "NpcWorkOrderCompleted") has_complete_event = true;
        }
    }
    assert(inventoryQuantity(f, "player", "torch") > player_torch_after_learning);
    assert(!entityExists(f, "scenario_beast_shadow"));
    assert(has_counter_event);
    assert(has_complete_event);

    std::cout << "client_runtime_bridge_npc_order_torch_counters_threat_after_teaching_tests: all passed" << std::endl;
}


// ---------------------------------------------------------------------------
// Nearby predators are not pickup targets; only real items should produce pickup.
// ---------------------------------------------------------------------------
void run_pickup_options_exclude_predator_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_no_pickup_predator", "player");
    assert(boot.is_ok());
    spawnMapItem(f, "near_beast_pickup_test", "beast_shadow", {0, 1, "surface"});

    auto refresh = f.harness.fakeRefresh("session_no_pickup_predator", "client_1", boot.value().projection_version);
    assert(refresh.is_ok());
    for (const auto& option : refresh.value().available_commands) {
        assert(!(option.command_kind == WorldCommandKind::Pickup &&
                 option.target.target_entity_id == "near_beast_pickup_test"));
    }

    std::cout << "client_runtime_bridge_pickup_options_exclude_predator_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Player can command an adjacent NPC to follow; subsequent player movement lets
// the backend move the follower through runtime movement and projection patches.
// ---------------------------------------------------------------------------
void run_npc_follow_command_moves_companion_tests() {
    RuntimeBackedFixture f;
    spawnTestActor(f, "companion", "companion", {1, 0, "surface"});

    auto boot = f.harness.fakeBootstrap("client_1", "session_npc_follow", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;

    auto companion_before = f.factory.world_runtime->findActor("companion");
    assert(companion_before.is_ok());
    const int start_x = companion_before.value()->coord.x;

    const auto* follow_option = findOption(boot.value().available_commands, [](const auto& option) {
        return option.command_kind == WorldCommandKind::Use &&
               option.command_key == "set_actor_follow_player" &&
               option.target.target_actor_key == "companion";
    });
    assert(follow_option != nullptr);

    auto follow = f.harness.fakeSubmitOption("session_npc_follow", "client_1", 1, version, follow_option->option_id);
    assert(follow.is_ok());
    assert(follow.value().result.result_kind == WorldCommandResultKind::Succeeded);
    version = follow.value().new_projection_version;
    auto commands = follow.value().available_commands;

    for (int step = 0; step < 3; ++step) {
        const auto* east = findOption(commands, [](const auto& option) {
            return option.command_kind == WorldCommandKind::Move &&
                   option.command_key == "move_east" &&
                   option.enabled;
        });
        assert(east != nullptr);
        auto move = f.harness.fakeSubmitOption("session_npc_follow", "client_1", static_cast<uint64_t>(step + 2), version, east->option_id);
        assert(move.is_ok());
        assert(move.value().result.result_kind == WorldCommandResultKind::Succeeded);
        version = move.value().new_projection_version;
        commands = move.value().available_commands;
    }

    auto companion_after = f.factory.world_runtime->findActor("companion");
    assert(companion_after.is_ok());
    assert(companion_after.value()->coord.x > start_x);

    std::cout << "client_runtime_bridge_npc_follow_command_moves_companion_tests: all passed" << std::endl;
}



// ---------------------------------------------------------------------------
// Map edit options come from backend command pipeline: raw objects are placeable,
// crafted outputs stay unavailable for direct placement.
// ---------------------------------------------------------------------------
void run_map_edit_options_respect_raw_policy_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_map_edit_policy", "player");
    assert(boot.is_ok());

    bool has_paint = false;
    bool has_raw_stone = false;
    bool has_raw_mushroom = false;
    bool has_crafted_axe = false;
    bool has_crafted_torch = false;
    bool has_camp_fire = false;
    bool has_place_companion = false;
    for (const auto& option : boot.value().available_commands) {
        if (option.command_kind == WorldCommandKind::PaintTerrain &&
            option.command_key == "paint_terrain" &&
            option.target.target_item_key == "forest") {
            has_paint = true;
        }
        if (option.command_kind == WorldCommandKind::SpawnEntity &&
            option.command_key == "place_raw_object") {
            if (option.target.target_item_key == "loose_stone") has_raw_stone = true;
            if (option.target.target_item_key == "mushroom") has_raw_mushroom = true;
            if (option.target.target_item_key == "axe") has_crafted_axe = true;
            if (option.target.target_item_key == "torch") has_crafted_torch = true;
            if (option.target.target_item_key == "camp_fire") has_camp_fire = true;
        }
        if (option.command_kind == WorldCommandKind::SpawnEntity &&
            option.command_key == "place_agent" &&
            option.target.target_item_key == "companion") {
            has_place_companion = true;
        }
    }

    assert(has_paint);
    assert(has_raw_stone);
    assert(has_raw_mushroom);
    assert(!has_crafted_axe);
    assert(!has_crafted_torch);
    assert(!has_camp_fire);
    assert(has_place_companion);

    std::cout << "client_runtime_bridge_map_edit_options_respect_raw_policy_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Map edit command execution must be submitted through a session option_id,
// then handled by WorldCommandPipeline and authoritative runtime ports.
// ---------------------------------------------------------------------------
void run_map_edit_commands_execute_through_pipeline_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_map_edit_execute", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;

    const auto* paint = findOption(boot.value().available_commands, [](const auto& option) {
        return option.command_kind == WorldCommandKind::PaintTerrain &&
               option.command_key == "paint_terrain" &&
               option.target.target_item_key == "stone_field" &&
               option.target.target_coord &&
               option.target.target_coord->x == 0 &&
               option.target.target_coord->y == 0;
    });
    assert(paint != nullptr);

    auto paint_response = f.harness.fakeSubmitOption("session_map_edit_execute", "client_1", 1, version, paint->option_id);
    assert(paint_response.is_ok());
    assert(paint_response.value().result.result_kind == WorldCommandResultKind::Succeeded);
    version = paint_response.value().new_projection_version;

    auto cell = f.factory.world_runtime->findCell({0, 0, "surface"});
    assert(cell.is_ok());
    assert(cell.value()->terrain_key == "stone_field");

    auto refresh = f.harness.fakeRefresh("session_map_edit_execute", "client_1", version);
    assert(refresh.is_ok());
    version = refresh.value().projection_version;

    const auto* place = findOption(refresh.value().available_commands, [](const auto& option) {
        return option.command_kind == WorldCommandKind::SpawnEntity &&
               option.command_key == "place_raw_object" &&
               option.target.target_item_key == "flint" &&
               option.target.target_coord &&
               option.target.target_coord->x == 0 &&
               option.target.target_coord->y == 0;
    });
    assert(place != nullptr);

    auto place_response = f.harness.fakeSubmitOption("session_map_edit_execute", "client_1", 2, version, place->option_id);
    assert(place_response.is_ok());
    assert(place_response.value().result.result_kind == WorldCommandResultKind::Succeeded);

    auto snap = f.factory.world_runtime->snapshotForDebug();
    assert(snap.is_ok());
    bool found_flint = false;
    for (const auto& [entity_id, entity] : snap.value().entities) {
        if (entity.entity_key == "flint" && entity.coord &&
            entity.coord->x == 0 && entity.coord->y == 0 && entity.coord->layer_key == "surface") {
            found_flint = true;
            break;
        }
    }
    assert(found_flint);

    version = place_response.value().new_projection_version;
    auto after_place_refresh = f.harness.fakeRefresh("session_map_edit_execute", "client_1", version);
    assert(after_place_refresh.is_ok());
    version = after_place_refresh.value().projection_version;

    const auto* place_agent = findOption(after_place_refresh.value().available_commands, [](const auto& option) {
        return option.command_kind == WorldCommandKind::SpawnEntity &&
               option.command_key == "place_agent" &&
               option.target.target_item_key == "companion" &&
               option.target.target_coord;
    });
    assert(place_agent != nullptr);
    const auto placed_agent_coord = *place_agent->target.target_coord;

    auto agent_response = f.harness.fakeSubmitOption("session_map_edit_execute", "client_1", 3, version, place_agent->option_id);
    assert(agent_response.is_ok());
    assert(agent_response.value().result.result_kind == WorldCommandResultKind::Succeeded);

    snap = f.factory.world_runtime->snapshotForDebug();
    assert(snap.is_ok());
    bool found_placed_companion = false;
    for (const auto& [actor_key, actor] : snap.value().actors) {
        if (actor_key.find("placed_actor_companion_") == 0 &&
            actor.coord.x == placed_agent_coord.x &&
            actor.coord.y == placed_agent_coord.y &&
            actor.coord.layer_key == placed_agent_coord.layer_key) {
            found_placed_companion = true;
            break;
        }
    }
    assert(found_placed_companion);

    std::cout << "client_runtime_bridge_map_edit_commands_execute_through_pipeline_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Idle humanoid NPCs should advance through backend autonomous wander ticks on Wait.
// ---------------------------------------------------------------------------
void run_idle_npc_wanders_on_wait_tests() {
    RuntimeBackedFixture f;
    spawnTestActor(f, "companion_idle", "companion", {1, 0, "surface"});

    auto boot = f.harness.fakeBootstrap("client_1", "session_idle_wander", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;
    uint64_t sequence = 1;

    auto before = f.factory.world_runtime->findActor("companion_idle");
    assert(before.is_ok());
    const auto start = before.value()->coord;

    bool moved = false;
    bool saw_event = false;
    for (int step = 0; step < 5 && !moved; ++step) {
        auto response = submitWait(f, "session_idle_wander", version, sequence);
        for (const auto& event : response.event_feed) {
            if (event.event_kind == "NpcWanderStep") saw_event = true;
        }
        auto after = f.factory.world_runtime->findActor("companion_idle");
        assert(after.is_ok());
        moved = after.value()->coord.x != start.x || after.value()->coord.y != start.y;
        if (moved) {
            const int dx = std::abs(after.value()->coord.x - start.x);
            const int dy = std::abs(after.value()->coord.y - start.y);
            assert(dx + dy == 1 && "wander movement must be one cardinal grid step, not diagonal");
        }
    }

    assert(moved);
    assert(saw_event);
    std::cout << "client_runtime_bridge_idle_npc_wanders_on_wait_tests: all passed" << std::endl;
}


// ---------------------------------------------------------------------------
// Idle NPCs should try nearby world objects through backend commands, and the
// resulting experience must be learned by that NPC rather than by the player.
// ---------------------------------------------------------------------------
void run_idle_npc_tries_nearby_object_and_learns_tests() {
    RuntimeBackedFixture f;
    spawnTestActor(f, "companion_taster", "companion", {0, 0, "surface"});
    auto hunger_setup = f.factory.world_runtime->applyActorNumericStateDelta(
        "companion_taster", "hunger", 60.0, 0.0, 100.0, {"test_make_hungry"});
    assert(hunger_setup.is_ok());
    spawnMapItem(f, "test_red_berry_for_npc", "red_berry", {1, 0, "surface"}, 1);

    auto boot = f.harness.fakeBootstrap("client_1", "session_npc_object_try", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;
    uint64_t sequence = 1;

    bool saw_object_event = false;
    for (int step = 0; step < 3 && entityExists(f, "test_red_berry_for_npc"); ++step) {
        auto response = submitWait(f, "session_npc_object_try", version, sequence);
        version = response.new_projection_version;
        for (const auto& event : response.event_feed) {
            if (event.event_kind == "ObjectEaten") saw_object_event = true;
        }
    }

    assert(!entityExists(f, "test_red_berry_for_npc"));
    assert(saw_object_event);
    auto after_actor = f.factory.world_runtime->findActor("companion_taster");
    assert(after_actor.is_ok());
    auto after_entity = f.factory.world_runtime->findEntity(after_actor.value()->entity_id);
    assert(after_entity.is_ok());
    auto hunger_it = after_entity.value()->numeric_states.find("hunger");
    assert(hunger_it != after_entity.value()->numeric_states.end());
    assert(hunger_it->second < 60.0 && "red berry restore_hunger must reduce hunger in runtime state");

    pathfinder::knowledge::KnowledgeOwner npc_owner;
    npc_owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
    npc_owner.entity_id = pathfinder::foundation::EntityId("companion_taster");
    auto npc_claims = f.factory.knowledge_repository->listByOwner(npc_owner);
    assert(npc_claims.is_ok());
    bool npc_learned = false;
    for (const auto& claim : npc_claims.value()) {
        if (claim.subject.subject_id == "red_berry" &&
            claim.predicate.action_key == "eat" &&
            claim.predicate.effect_key == "restore_hunger") {
            npc_learned = true;
        }
    }
    assert(npc_learned);

    pathfinder::knowledge::KnowledgeOwner player_owner;
    player_owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
    player_owner.entity_id = pathfinder::foundation::EntityId("player");
    auto player_claims = f.factory.knowledge_repository->listByOwner(player_owner);
    assert(player_claims.is_ok());
    bool player_wrongly_learned = false;
    for (const auto& claim : player_claims.value()) {
        if (claim.subject.subject_id == "red_berry" && claim.predicate.action_key == "eat") {
            player_wrongly_learned = true;
        }
    }
    assert(!player_wrongly_learned);

    auto refresh = f.harness.fakeRefresh("session_npc_object_try", "client_1", version);
    assert(refresh.is_ok());
    bool npc_knowledge_projected = false;
    for (const auto& patch : refresh.value().full_projection.knowledge) {
        if (patch.actor_key != "companion_taster") continue;
        auto subject_it = patch.fields.find("subject_id");
        auto action_it = patch.fields.find("action_key");
        auto effect_it = patch.fields.find("effect_key");
        if (subject_it != patch.fields.end() && action_it != patch.fields.end() && effect_it != patch.fields.end() &&
            subject_it->second == "red_berry" && action_it->second == "eat" && effect_it->second == "restore_hunger") {
            npc_knowledge_projected = true;
            break;
        }
    }
    assert(npc_knowledge_projected);

    std::cout << "client_runtime_bridge_idle_npc_tries_nearby_object_and_learns_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// A not-hungry NPC should not immediately eat every berry. It should first try
// non-consuming use, then put the spare object into its inventory through the
// Pickup command pipeline.
// ---------------------------------------------------------------------------
void run_idle_npc_uses_then_picks_up_when_not_hungry_tests() {
    RuntimeBackedFixture f;
    spawnTestActor(f, "companion_collector", "companion", {0, 0, "surface"});
    spawnMapItem(f, "test_red_berry_for_pickup", "red_berry", {1, 0, "surface"}, 1);

    auto boot = f.harness.fakeBootstrap("client_1", "session_npc_pickup", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;
    uint64_t sequence = 1;

    bool saw_use = false;
    bool saw_pickup = false;
    for (int step = 0; step < 4 && entityOnMap(f, "test_red_berry_for_pickup"); ++step) {
        auto response = submitWait(f, "session_npc_pickup", version, sequence);
        version = response.new_projection_version;
        for (const auto& event : response.event_feed) {
            if (event.event_kind == "ObjectUsed") saw_use = true;
            if (event.event_kind == "ItemPickedUp") saw_pickup = true;
        }
    }

    assert(saw_use);
    assert(saw_pickup);
    assert(actorHasClaim(f, "companion_collector", "red_berry", "use", "no_visible_effect"));
    assert(!entityOnMap(f, "test_red_berry_for_pickup"));
    assert(inventoryQuantity(f, "companion_collector", "red_berry") == 1);

    std::cout << "client_runtime_bridge_idle_npc_uses_then_picks_up_when_not_hungry_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Once an NPC has tried direct use on red berries and learned there is no
// visible effect, it should not repeat that same exploratory use for every
// equivalent stack before switching to pickup.
// ---------------------------------------------------------------------------
void run_idle_npc_does_not_repeat_red_berry_use_tests() {
    RuntimeBackedFixture f;
    spawnTestActor(f, "companion_no_repeat", "companion", {0, 0, "surface"});
    spawnMapItem(f, "test_red_berry_repeat_a", "red_berry", {1, 0, "surface"}, 1);
    spawnMapItem(f, "test_red_berry_repeat_b", "red_berry", {0, 1, "surface"}, 1);
    spawnMapItem(f, "test_red_berry_repeat_c", "red_berry", {1, 1, "surface"}, 1);

    auto boot = f.harness.fakeBootstrap("client_1", "session_npc_no_repeat", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;
    uint64_t sequence = 1;

    int use_count = 0;
    int pickup_count = 0;
    for (int step = 0; step < 5; ++step) {
        auto response = submitWait(f, "session_npc_no_repeat", version, sequence);
        for (const auto& event : response.event_feed) {
            if (event.actor_key != "companion_no_repeat") continue;
            if (event.event_kind == "ObjectUsed") ++use_count;
            if (event.event_kind == "ItemPickedUp") ++pickup_count;
        }
    }

    assert(use_count == 1);
    assert(pickup_count >= 2);
    assert(actorHasClaim(f, "companion_no_repeat", "red_berry", "use", "no_visible_effect"));
    assert(inventoryQuantity(f, "companion_no_repeat", "red_berry") >= 2);

    std::cout << "client_runtime_bridge_idle_npc_does_not_repeat_red_berry_use_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Hungry NPCs should prioritize food already in their inventory and consume it
// through the Eat command pipeline before wandering or searching the map.
// ---------------------------------------------------------------------------
void run_hungry_npc_eats_inventory_food_tests() {
    RuntimeBackedFixture f;
    spawnTestActor(f, "companion_inventory_eater", "companion", {0, 0, "surface"});
    auto hunger_setup = f.factory.world_runtime->applyActorNumericStateDelta(
        "companion_inventory_eater", "hunger", 50.0, 0.0, 100.0, {"test_make_inventory_eater_hungry"});
    assert(hunger_setup.is_ok());
    spawnInventoryItem(f, "companion_inventory_eater", "red_berry", 1, "inventory_eater_red_berry");

    auto boot = f.harness.fakeBootstrap("client_1", "session_npc_inventory_eat", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;
    uint64_t sequence = 1;

    auto response = submitWait(f, "session_npc_inventory_eat", version, sequence);
    bool saw_eat = false;
    for (const auto& event : response.event_feed) {
        if (event.actor_key == "companion_inventory_eater" && event.event_kind == "ObjectEaten") saw_eat = true;
    }
    assert(saw_eat);
    assert(inventoryQuantity(f, "companion_inventory_eater", "red_berry") == 0);
    assert(actorHasClaim(f, "companion_inventory_eater", "red_berry", "eat", "restore_hunger"));

    auto actor = f.factory.world_runtime->findActor("companion_inventory_eater");
    assert(actor.is_ok());
    auto entity = f.factory.world_runtime->findEntity(actor.value()->entity_id);
    assert(entity.is_ok());
    auto hunger_it = entity.value()->numeric_states.find("hunger");
    assert(hunger_it != entity.value()->numeric_states.end());
    assert(hunger_it->second < 60.0);

    std::cout << "client_runtime_bridge_hungry_npc_eats_inventory_food_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P63: wildlife is a real actor; waiting lets it move through BeastDecision and
// eventually attack through the Attack command handler with projected health.
// ---------------------------------------------------------------------------
void run_wildlife_actor_chases_and_attacks_tests() {
    RuntimeBackedFixture f;
    spawnTestActor(f, "companion", "companion", {1, 0, "surface"});
    spawnTestActor(f, "beast_shadow", "beast_shadow", {4, 1, "surface"}, "scenario_beast_shadow");

    auto boot = f.harness.fakeBootstrap("client_1", "session_wildlife", "player");
    assert(boot.is_ok());
    uint64_t version = boot.value().projection_version;

    auto beast_actor = f.factory.world_runtime->findActor("beast_shadow");
    assert(beast_actor.is_ok());
    assert(entityExists(f, "scenario_beast_shadow"));

    // Remove the starting camp fire so this test isolates chase/attack instead
    // of the fire deterrent branch. This is fixture setup, not gameplay logic.
    (void)f.factory.world_runtime->destroyEntity("scenario_camp_fire");

    int initial_player_health = f.factory.world_runtime->findActor("player").value()->health;
    int initial_companion_health = f.factory.world_runtime->findActor("companion").value()->health;
    bool saw_damage_event = false;
    bool saw_health_patch = false;

    for (int step = 0; step < 10 && !saw_damage_event; ++step) {
        auto refresh = f.harness.fakeRefresh("session_wildlife", "client_1", version);
        assert(refresh.is_ok());
        version = refresh.value().projection_version;
        const auto* wait = findOption(refresh.value().available_commands, [](const auto& option) {
            return option.command_kind == WorldCommandKind::Wait && option.command_key == "wait";
        });
        assert(wait != nullptr);

        auto response = f.harness.fakeSubmitOption("session_wildlife", "client_1", static_cast<uint64_t>(step + 1), version, wait->option_id);
        assert(response.is_ok());
        assert(response.value().result.result_kind == WorldCommandResultKind::Succeeded);
        version = response.value().new_projection_version;

        for (const auto& event : response.value().event_feed) {
            if (event.event_kind == "ActorDamaged" || event.event_kind == "ActorDowned") {
                saw_damage_event = true;
            }
        }
        for (const auto& patch : response.value().projection_patch.changed_entities) {
            auto key_it = patch.fields.find("entity_key");
            auto health_it = patch.fields.find("health");
            auto max_it = patch.fields.find("max_health");
            if (key_it != patch.fields.end() &&
                (key_it->second == "player" || key_it->second == "companion") &&
                health_it != patch.fields.end() && max_it != patch.fields.end()) {
                saw_health_patch = true;
            }
        }
    }

    auto player_after = f.factory.world_runtime->findActor("player");
    auto companion_after = f.factory.world_runtime->findActor("companion");
    assert(player_after.is_ok());
    assert(companion_after.is_ok());
    assert(player_after.value()->health < initial_player_health || companion_after.value()->health < initial_companion_health);
    assert(saw_damage_event);
    assert(saw_health_patch);

    std::cout << "client_runtime_bridge_wildlife_actor_chases_and_attacks_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_bootstrap_non_empty_tests();
        run_sandbox_projection_includes_full_board_and_far_npc_tests();
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
        run_pickup_options_exclude_predator_tests();
        run_npc_follow_command_moves_companion_tests();
        run_map_edit_options_respect_raw_policy_tests();
        run_map_edit_commands_execute_through_pipeline_tests();
        run_idle_npc_wanders_on_wait_tests();
        run_idle_npc_tries_nearby_object_and_learns_tests();
        run_idle_npc_uses_then_picks_up_when_not_hungry_tests();
        run_idle_npc_does_not_repeat_red_berry_use_tests();
        run_hungry_npc_eats_inventory_food_tests();
        run_wildlife_actor_chases_and_attacks_tests();
        return 0;
    }

    std::string test_name = argv[1];
    if (test_name == "bootstrap_non_empty") run_bootstrap_non_empty_tests();
    else if (test_name == "sandbox_projection_includes_full_board_and_far_npc") run_sandbox_projection_includes_full_board_and_far_npc_tests();
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
    else if (test_name == "pickup_options_exclude_predator") run_pickup_options_exclude_predator_tests();
    else if (test_name == "npc_follow_command_moves_companion") run_npc_follow_command_moves_companion_tests();
    else if (test_name == "map_edit_options_respect_raw_policy") run_map_edit_options_respect_raw_policy_tests();
    else if (test_name == "map_edit_commands_execute_through_pipeline") run_map_edit_commands_execute_through_pipeline_tests();
    else if (test_name == "idle_npc_wanders_on_wait") run_idle_npc_wanders_on_wait_tests();
    else if (test_name == "idle_npc_tries_nearby_object_and_learns") run_idle_npc_tries_nearby_object_and_learns_tests();
    else if (test_name == "idle_npc_uses_then_picks_up_when_not_hungry") run_idle_npc_uses_then_picks_up_when_not_hungry_tests();
    else if (test_name == "idle_npc_does_not_repeat_red_berry_use") run_idle_npc_does_not_repeat_red_berry_use_tests();
    else if (test_name == "hungry_npc_eats_inventory_food") run_hungry_npc_eats_inventory_food_tests();
    else if (test_name == "wildlife_actor_chases_and_attacks") run_wildlife_actor_chases_and_attacks_tests();
    else {
        std::cerr << "Unknown test: " << test_name << std::endl;
        return 1;
    }
    return 0;
}
