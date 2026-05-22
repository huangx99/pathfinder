#include <cassert>
#include <iostream>
#include <string>
#include "pathfinder/client_http/client_server_runtime_factory.h"
#include "pathfinder/client_protocol/client_protocol_harness.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/client_http/client_http_types.h"
#include "json.hpp"

using namespace pathfinder::client_http;
using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::world_map_interaction;
using namespace pathfinder::world_region_state;
using namespace pathfinder::foundation;

struct RuntimeBackedFixture {
    ClientServerRuntimeFactory factory;
    ClientProtocolHarness harness;

    RuntimeBackedFixture()
        : harness(factory.session_gateway, factory.command_gateway, factory.codec) {}
};

// ---------------------------------------------------------------------------
// P60 Integration: Seal/Detach/Restore via fake client (formal command gateway)
// ---------------------------------------------------------------------------
// This test exercises the full P60 lifecycle through the production client
// protocol stack, NOT by directly manipulating runtime arrays.
// ---------------------------------------------------------------------------

static std::string findOptionIdByKind(
    const std::vector<WorldCommandOptionDto>& options,
    WorldCommandKind kind) {
    for (const auto& opt : options) {
        if (opt.command_kind == kind && opt.enabled) {
            return opt.option_id;
        }
    }
    return "";
}

static std::string findMoveOptionId(const ClientBootstrapResponse& boot, const std::string& command_key) {
    for (const auto& opt : boot.available_commands) {
        if (opt.command_key == command_key && opt.enabled) {
            return opt.option_id;
        }
    }
    return "";
}

static std::string findPickupOptionIdForEntity(
    const std::vector<WorldCommandOptionDto>& options,
    const std::string& entity_id) {
    for (const auto& opt : options) {
        if (opt.command_kind == WorldCommandKind::Pickup &&
            opt.enabled &&
            opt.target.target_entity_id == entity_id) {
            return opt.option_id;
        }
    }
    return "";
}

static bool hasEnabledPickupForEntity(
    const std::vector<WorldCommandOptionDto>& options,
    const std::string& entity_id) {
    return !findPickupOptionIdForEntity(options, entity_id).empty();
}

static bool hasLifecycleEvent(const ClientCommandResponse& resp, const std::string& kind_suffix) {
    for (const auto& ev : resp.event_feed) {
        if (ev.event_kind.find("Lifecycle:") == 0 && ev.event_kind.find(kind_suffix) != std::string::npos) {
            return true;
        }
    }
    return false;
}

static bool hasLifecycleEvent(const ClientBootstrapResponse& resp, const std::string& kind_suffix) {
    for (const auto& ev : resp.event_feed) {
        if (ev.event_kind.find("Lifecycle:") == 0 && ev.event_kind.find(kind_suffix) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void run_map_interaction_seal_restore_flow_tests() {
    RuntimeBackedFixture f;

    // P60: Shrink activity window so we can test seal/detach with few moves.
    // Region (0,0) covers x=[-8,7]. With vision=0 and margin=0, keep_active
    // contains only the region the actor is standing in. Moving to x=8
    // enters region (1,0) and drops (0,0) from keep_active.
    {
        RegionActivityWindowService::Config test_config;
        test_config.vision_radius_cells = 0;
        test_config.preload_margin_regions = 0;
        test_config.seal_delay_ticks = 0;
        test_config.detach_delay_ticks = 0;
        test_config.recently_touched_window_ticks = 0;
        f.factory.activity_window_service->setConfig(test_config);
    }

    // 1. Bootstrap
    auto boot = f.harness.fakeBootstrap("client_p60", "session_p60", "player");
    assert(boot.is_ok());
    assert(!boot.value().full_projection.visible_cells.empty());
    assert(boot.value().projection_version > 0);
    // P60: bootstrap should trigger lifecycle (prewarm / no seal yet because player is in origin)
    assert(!hasLifecycleEvent(boot.value(), "seal_failed")); // seal should not fail on bootstrap

    uint64_t version = boot.value().projection_version;
    auto current_boot = boot.value();

    // 2. Move player east 8 times to cross from region (0,0) into region (1,0)
    for (int step = 1; step <= 8; ++step) {
        std::string east_opt = findMoveOptionId(current_boot, "move_east");
        assert(!east_opt.empty() && "East move option must be available");

        auto cmd = f.harness.fakeSubmitOption(
            "session_p60", "client_p60", static_cast<uint64_t>(step), version, east_opt);
        assert(cmd.is_ok());
        assert(cmd.value().result.result_kind == WorldCommandResultKind::Succeeded);
        version = cmd.value().new_projection_version;
        current_boot.available_commands = cmd.value().available_commands;
    }

    // 3. Verify origin region (0,0) is sealed
    using pathfinder::world_generation::WorldRegionKey;
    WorldRegionKey origin_key;
    origin_key.world_id = "world_default";
    origin_key.layer_key = "surface";
    origin_key.rx = 0;
    origin_key.ry = 0;
    origin_key.region_size = 16;

    // Verify seal succeeded by checking store has snapshot and runtime no longer has the region
    assert(f.factory.region_store->has(origin_key) &&
           "Store should contain snapshot for origin region after seal");
    assert(!f.factory.world_runtime->isRegionGenerated(origin_key.regionRuntimeId()) &&
           "Origin region should be detached from runtime after seal");

    // 4. Verify store has the snapshot
    assert(f.factory.region_store->has(origin_key) &&
           "Store should contain the sealed snapshot for origin region");

    // 5. Move back west 8 times into origin region (0,0)
    bool found_restored = false;
    for (int step = 1; step <= 8; ++step) {
        std::string west_opt = findMoveOptionId(current_boot, "move_west");
        assert(!west_opt.empty() && "West move option must be available");

        auto cmd = f.harness.fakeSubmitOption(
            "session_p60", "client_p60", static_cast<uint64_t>(100 + step), version, west_opt);
        assert(cmd.is_ok());
        assert(cmd.value().result.result_kind == WorldCommandResultKind::Succeeded);
        version = cmd.value().new_projection_version;
        current_boot.available_commands = cmd.value().available_commands;

        // When re-entering origin region, ensureForTargetVision restores (0,0) from snapshot.
        if (hasLifecycleEvent(cmd.value(), "restored")) {
            found_restored = true;
        }
    }
    assert(found_restored && "Re-entering origin region should trigger restore from snapshot");

    // 6. Verify origin region is ActiveRuntime again (restored, not regenerated)
    auto origin_state_after = f.factory.lifecycle_service->getLifecycleState(origin_key);
    assert(origin_state_after == WorldRegionLifecycleState::ActiveRuntime &&
           "Origin region should be ActiveRuntime after restore");

    // 7. Verify player is back at origin
    auto actor_res = f.factory.world_runtime->findActor("player");
    assert(actor_res.is_ok());
    assert(actor_res.value()->coord.x == 0 && actor_res.value()->coord.y == 0 &&
           "Player should be back at origin after round-trip");

    std::cout << "map_interaction_seal_restore_flow: passed" << std::endl;
}


// ---------------------------------------------------------------------------
// P60 Integration: formal factory exposes inventory options through option bridge
// ---------------------------------------------------------------------------

void run_map_interaction_inventory_option_flow_tests() {
    RuntimeBackedFixture f;

    auto actor = f.factory.world_runtime->findActor("player");
    assert(actor.is_ok());
    auto coord = actor.value()->coord;

    auto spawn_res = f.factory.world_runtime->spawnEntityOnMap(
        "p60_pickup_item_1",
        "loose_stone",
        "entity.loose_stone",
        coord,
        1,
        "loose_stone:default",
        true,
        {}, {}, {});
    assert(spawn_res.is_ok());

    auto boot = f.harness.fakeBootstrap("client_p60_inv", "session_p60_inv", "player");
    assert(boot.is_ok());
    assert(!hasEnabledPickupForEntity(boot.value().available_commands, actor.value()->entity_id) &&
           "Actor entity must never be exposed as a pickup target");

    std::string pickup_opt = findPickupOptionIdForEntity(
        boot.value().available_commands,
        "p60_pickup_item_1");
    assert(!pickup_opt.empty() && "Factory option bridge must include pickup for the spawned item");

    auto pickup = f.harness.fakeSubmitOption(
        "session_p60_inv",
        "client_p60_inv",
        1,
        boot.value().projection_version,
        pickup_opt);
    assert(pickup.is_ok());
    assert(pickup.value().result.result_kind == WorldCommandResultKind::Succeeded);

    bool removed_from_projection = false;
    for (const auto& patch : pickup.value().projection_patch.changed_entities) {
        if (patch.entity_id == "p60_pickup_item_1" && patch.op == PatchOp::Remove) {
            removed_from_projection = true;
            break;
        }
    }
    assert(removed_from_projection && "Pickup must send entity remove patch so the client removes the map item");
    assert(!hasEnabledPickupForEntity(pickup.value().available_commands, "p60_pickup_item_1") &&
           "Picked item must not remain as an available pickup target");

    auto refresh = f.harness.fakeRefresh(
        "session_p60_inv",
        "client_p60_inv",
        pickup.value().new_projection_version);
    assert(refresh.is_ok());
    for (const auto& entity : refresh.value().full_projection.visible_entities) {
        assert(entity.entity_id != "p60_pickup_item_1" &&
               "Picked item must disappear from refreshed projection");
    }

    std::string drop_opt = findOptionIdByKind(pickup.value().available_commands, WorldCommandKind::Drop);
    assert(!drop_opt.empty() && "Factory option bridge must include inventory drop provider after pickup");

    std::cout << "map_interaction_inventory_option_flow: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P60 Integration: stale selection must not overwrite authoritative options
// ---------------------------------------------------------------------------

void run_map_interaction_stale_selection_no_snapshot_pollution_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_p60_stale", "session_p60_stale", "player");
    assert(boot.is_ok());
    auto before_snapshot = f.factory.session_gateway.getAvailableCommands("session_p60_stale");
    assert(before_snapshot.is_ok());
    assert(!before_snapshot.value().empty());

    auto actor = f.factory.world_runtime->findActor("player");
    assert(actor.is_ok());
    auto coord = actor.value()->coord;

    auto spawn_res = f.factory.world_runtime->spawnEntityOnMap(
        "p60_stale_item_1",
        "loose_stone",
        "entity.loose_stone",
        coord,
        1,
        "loose_stone:default",
        true,
        {}, {}, {});
    assert(spawn_res.is_ok());
    assert(f.factory.world_runtime->stateVersion() != boot.value().projection_version);

    nlohmann::json body;
    body["session_id"] = "session_p60_stale";
    body["client_id"] = "client_p60_stale";
    body["known_projection_version"] = boot.value().projection_version;
    body["layer_key"] = "surface";
    body["selection_kind"] = "Cell";
    body["x"] = coord.x;
    body["y"] = coord.y;

    ClientHttpRequest req;
    req.method = ClientHttpMethod::Post;
    req.path = "/api/client/select";
    req.body = body.dump();

    auto resp = f.factory.http_gateway->handleApi(req);
    assert(resp.status_code == 200);
    auto parsed = nlohmann::json::parse(resp.body);
    assert(parsed.value("stale", false));
    assert(parsed.value("requires_full_refresh", false));
    assert(parsed.at("available_options").empty());

    auto after_snapshot = f.factory.session_gateway.getAvailableCommands("session_p60_stale");
    assert(after_snapshot.is_ok());
    assert(after_snapshot.value().size() == before_snapshot.value().size());
    for (size_t i = 0; i < before_snapshot.value().size(); ++i) {
        assert(after_snapshot.value()[i].option_id == before_snapshot.value()[i].option_id);
    }

    std::cout << "map_interaction_stale_selection_no_snapshot_pollution: passed" << std::endl;
}
