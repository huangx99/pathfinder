#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/world_inventory/world_inventory_command_handlers.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_command;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;
using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// Helper fixture
// ---------------------------------------------------------------------------

class IntegrationFixture {
public:
    WorldRuntimeConfig config;
    WorldGridRuntime world_runtime;
    std::unique_ptr<WorldInventoryRuntime> inventory_runtime;
    std::unique_ptr<WorldCommandHandlerRegistry> registry;
    std::unique_ptr<WorldCommandDispatcher> dispatcher;
    std::unique_ptr<WorldCommandPipeline> pipeline;

    void setup() {
        config.seed = 1; // seed=1 has item at player origin cell
        config.region_size = 9;
        config.initial_region_radius = 0;
        config.default_vision_radius = 3;

        world_runtime.initialize(config);
        world_runtime.generateInitialWorld(config);

        inventory_runtime = std::make_unique<WorldInventoryRuntime>(world_runtime);
        inventory_runtime->initialize();

        registry = std::make_unique<WorldCommandHandlerRegistry>();
        registry->registerHandler(createNoopCommandHandler());
        registry->registerHandler(createWaitCommandHandler());
        registry->registerHandler(createInspectCommandHandler());
        registry->registerHandler(createSystemTickCommandHandler());
        registry->registerHandler(createGenerateWorldCommandHandler());

        // P45: Register Pickup and Drop handlers
        registry->registerHandler(createPickupCommandHandler(*inventory_runtime, world_runtime));
        registry->registerHandler(createDropCommandHandler(*inventory_runtime, world_runtime));

        dispatcher = std::make_unique<WorldCommandDispatcher>(*registry);
        pipeline = std::make_unique<WorldCommandPipeline>(*dispatcher);
    }

    std::string findGroundItem() const {
        auto player_coord = playerCoord();
        auto snap = world_runtime.snapshotForDebug();
        if (snap.is_ok()) {
            for (const auto& [id, entity] : snap.value().entities) {
                if (entity.location_kind == WorldEntityLocationKind::OnMap &&
                    entity.entity_key != "player" && entity.coord.has_value()) {
                    int dx = std::abs(entity.coord.value().x - player_coord.x);
                    int dy = std::abs(entity.coord.value().y - player_coord.y);
                    if ((dx == 0 && dy == 0) || (dx == 1 && dy == 0) || (dx == 0 && dy == 1)) {
                        return id;
                    }
                }
            }
        }
        auto spawn_res = const_cast<WorldGridRuntime&>(world_runtime).spawnEntityOnMap(
            "test_adjacent_pickup_item", "loose_stone", "entity.loose_stone",
            player_coord, 1, "loose_stone:default", true, {}, {}, {});
        if (spawn_res.is_ok()) {
            return "test_adjacent_pickup_item";
        }
        return "";
    }

    WorldCellCoord playerCoord() const {
        auto actor_res = world_runtime.findActor("player");
        assert(actor_res.is_ok());
        return actor_res.value()->coord;
    }
};

// ---------------------------------------------------------------------------
// Pickup via pipeline
// ---------------------------------------------------------------------------

void run_command_pickup_runtime_tests() {
    IntegrationFixture f;
    f.setup();

    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    WorldCommandDto cmd;
    cmd.command_id = "cmd_pickup_1";
    cmd.command_key = "pickup";
    cmd.command_kind = WorldCommandKind::Pickup;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";
    cmd.target.target_kind = WorldCommandTargetKind::Entity;
    cmd.target.target_entity_id = item_id;

    auto result = f.pipeline->execute("session_p45", cmd);
    assert(result.is_ok());
    auto response = result.value();
    assert(response.result.result_kind == WorldCommandResultKind::Succeeded);
    assert(!response.event_feed.empty());
    assert(response.event_feed[0].event_kind == "ItemPickedUp");

    // Projection patch must contain changed_inventories
    assert(!response.projection_patch.changed_inventories.empty());

    std::cout << "world_command_pickup_runtime: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Drop via pipeline
// ---------------------------------------------------------------------------

void run_command_drop_runtime_tests() {
    IntegrationFixture f;
    f.setup();

    // Pickup first
    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    WorldCommandDto pickup_cmd;
    pickup_cmd.command_id = "cmd_pickup_for_drop";
    pickup_cmd.command_key = "pickup";
    pickup_cmd.command_kind = WorldCommandKind::Pickup;
    pickup_cmd.source = WorldCommandSource::PlayerInput;
    pickup_cmd.actor_key = "player";
    pickup_cmd.target.target_kind = WorldCommandTargetKind::Entity;
    pickup_cmd.target.target_entity_id = item_id;

    auto pres = f.pipeline->execute("session_p45", pickup_cmd);
    assert(pres.is_ok());
    assert(pres.value().result.result_kind == WorldCommandResultKind::Succeeded);

    // Now drop
    WorldCommandDto drop_cmd;
    drop_cmd.command_id = "cmd_drop_1";
    drop_cmd.command_key = "drop";
    drop_cmd.command_kind = WorldCommandKind::Drop;
    drop_cmd.source = WorldCommandSource::PlayerInput;
    drop_cmd.actor_key = "player";
    drop_cmd.parameters["entity_id"] = item_id;
    drop_cmd.parameters["quantity"] = "1";

    auto result = f.pipeline->execute("session_p45", drop_cmd);
    assert(result.is_ok());
    auto response = result.value();
    assert(response.result.result_kind == WorldCommandResultKind::Succeeded);
    assert(!response.event_feed.empty());
    assert(response.event_feed[0].event_kind == "ItemDropped");

    // Projection patch must contain changed_cells, changed_entities, changed_inventories
    assert(!response.projection_patch.changed_cells.empty());
    assert(!response.projection_patch.changed_entities.empty());
    assert(!response.projection_patch.changed_inventories.empty());

    std::cout << "world_command_drop_runtime: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Pickup too far via pipeline
// ---------------------------------------------------------------------------

void run_command_pickup_too_far_tests() {
    IntegrationFixture f;
    f.setup();

    auto snap = f.world_runtime.snapshotForDebug();
    assert(snap.is_ok());

    std::string far_item;
    auto player_coord = f.playerCoord();
    for (const auto& [id, entity] : snap.value().entities) {
        if (entity.location_kind == WorldEntityLocationKind::OnMap && entity.coord.has_value()) {
            int dx = std::abs(entity.coord.value().x - player_coord.x);
            int dy = std::abs(entity.coord.value().y - player_coord.y);
            if (dx > 1 || dy > 1) {
                far_item = id;
                break;
            }
        }
    }

    if (!far_item.empty()) {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_pickup_far";
        cmd.command_key = "pickup";
        cmd.command_kind = WorldCommandKind::Pickup;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        cmd.target.target_kind = WorldCommandTargetKind::Entity;
        cmd.target.target_entity_id = far_item;

        auto result = f.pipeline->execute("session_p45", cmd);
        assert(result.is_ok());
        auto response = result.value();
        assert(response.result.result_kind == WorldCommandResultKind::Blocked);
        assert(!response.result.failure_reason_keys.empty());
    }

    std::cout << "world_command_pickup_too_far: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Pickup missing item via pipeline
// ---------------------------------------------------------------------------

void run_command_pickup_missing_item_tests() {
    IntegrationFixture f;
    f.setup();

    WorldCommandDto cmd;
    cmd.command_id = "cmd_pickup_missing";
    cmd.command_key = "pickup";
    cmd.command_kind = WorldCommandKind::Pickup;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";
    cmd.target.target_kind = WorldCommandTargetKind::Entity;
    cmd.target.target_entity_id = "nonexistent_xyz";

    auto result = f.pipeline->execute("session_p45", cmd);
    assert(result.is_ok());
    auto response = result.value();
    assert(response.result.result_kind == WorldCommandResultKind::Blocked);
    assert(!response.result.failure_reason_keys.empty());

    std::cout << "world_command_pickup_missing_item: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Drop not owned via pipeline
// ---------------------------------------------------------------------------

void run_command_drop_not_owned_tests() {
    IntegrationFixture f;
    f.setup();

    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    // Try to drop without picking up
    WorldCommandDto cmd;
    cmd.command_id = "cmd_drop_not_owned";
    cmd.command_key = "drop";
    cmd.command_kind = WorldCommandKind::Drop;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";
    cmd.parameters["entity_id"] = item_id;
    cmd.parameters["quantity"] = "1";

    auto result = f.pipeline->execute("session_p45", cmd);
    assert(result.is_ok());
    auto response = result.value();
    assert(response.result.result_kind == WorldCommandResultKind::Blocked);
    assert(!response.result.failure_reason_keys.empty());

    std::cout << "world_command_drop_not_owned: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Inventory projection patch entries_json tests
// ---------------------------------------------------------------------------

void run_command_inventory_projection_entries_json_tests() {
    IntegrationFixture f;
    f.setup();

    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    WorldCommandDto cmd;
    cmd.command_id = "cmd_pickup_proj";
    cmd.command_key = "pickup";
    cmd.command_kind = WorldCommandKind::Pickup;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";
    cmd.target.target_kind = WorldCommandTargetKind::Entity;
    cmd.target.target_entity_id = item_id;

    auto result = f.pipeline->execute("session_p45", cmd);
    assert(result.is_ok());
    auto response = result.value();
    assert(!response.projection_patch.changed_inventories.empty());

    auto& inv_patch = response.projection_patch.changed_inventories[0];
    auto it = inv_patch.fields.find("entries_json");
    assert(it != inv_patch.fields.end());
    assert(!it->second.empty());
    // Verify JSON structure contains required fields
    assert(it->second.find("entry_id") != std::string::npos);
    assert(it->second.find("entity_id") != std::string::npos);
    assert(it->second.find("entity_key") != std::string::npos);
    assert(it->second.find("stack_key") != std::string::npos);
    assert(it->second.find("quantity") != std::string::npos);
    assert(it->second.find("location_kind") != std::string::npos);

    std::cout << "world_command_inventory_projection_entries_json: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests:" << std::endl;
        std::cout << "  pickup_runtime" << std::endl;
        std::cout << "  drop_runtime" << std::endl;
        std::cout << "  pickup_too_far" << std::endl;
        std::cout << "  pickup_missing_item" << std::endl;
        std::cout << "  drop_not_owned" << std::endl;
        std::cout << "  inventory_projection_entries_json" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "pickup_runtime") {
        run_command_pickup_runtime_tests();
    } else if (test_name == "drop_runtime") {
        run_command_drop_runtime_tests();
    } else if (test_name == "pickup_too_far") {
        run_command_pickup_too_far_tests();
    } else if (test_name == "pickup_missing_item") {
        run_command_pickup_missing_item_tests();
    } else if (test_name == "drop_not_owned") {
        run_command_drop_not_owned_tests();
    } else if (test_name == "inventory_projection_entries_json") {
        run_command_inventory_projection_entries_json_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
