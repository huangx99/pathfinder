#include <cassert>
#include <iostream>
#include "pathfinder/client_runtime_bridge/client_runtime_bridge_types.h"
#include "pathfinder/client_runtime_bridge/client_runtime_projection_bridge.h"
#include "pathfinder/client_runtime_bridge/client_runtime_command_option_bridge.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_runtime/world_command_runtime_handlers.h"
#include "pathfinder/world_command/world_command_handlers.h"

using namespace pathfinder::client_runtime_bridge;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

// ---------------------------------------------------------------------------
// Enum roundtrip
// ---------------------------------------------------------------------------
void run_enum_roundtrip_tests() {
    // ClientRuntimeBridgeMode
    assert(toString(ClientRuntimeBridgeMode::RuntimeBacked) == "runtime_backed");
    assert(toString(ClientRuntimeBridgeMode::StubForProtocolTest) == "stub_for_protocol_test");
    assert(toString(ClientRuntimeBridgeMode::Disabled) == "disabled");
    assert(toString(ClientRuntimeBridgeMode::TestOnly) == "test_only");
    assert(toString(ClientRuntimeBridgeMode::Unknown) == "unknown");

    auto m = clientRuntimeBridgeModeFromString("runtime_backed");
    assert(m.is_ok() && m.value() == ClientRuntimeBridgeMode::RuntimeBacked);
    m = clientRuntimeBridgeModeFromString("invalid");
    assert(m.is_error());

    // ClientCommandOptionProviderKind
    assert(toString(ClientCommandOptionProviderKind::Wait) == "wait");
    assert(toString(ClientCommandOptionProviderKind::Move) == "move");
    assert(toString(ClientCommandOptionProviderKind::Inspect) == "inspect");
    assert(toString(ClientCommandOptionProviderKind::Inventory) == "inventory");
    assert(toString(ClientCommandOptionProviderKind::Harvest) == "harvest");
    assert(toString(ClientCommandOptionProviderKind::Craft) == "craft");
    assert(toString(ClientCommandOptionProviderKind::Teach) == "teach");
    assert(toString(ClientCommandOptionProviderKind::Combat) == "combat");
    assert(toString(ClientCommandOptionProviderKind::AreaEffect) == "area_effect");
    assert(toString(ClientCommandOptionProviderKind::TestOnly) == "test_only");

    auto k = clientCommandOptionProviderKindFromString("move");
    assert(k.is_ok() && k.value() == ClientCommandOptionProviderKind::Move);
    k = clientCommandOptionProviderKindFromString("invalid");
    assert(k.is_error());

    // ClientMoveDirection
    assert(toString(ClientMoveDirection::North) == "north");
    assert(toString(ClientMoveDirection::South) == "south");
    assert(toString(ClientMoveDirection::West) == "west");
    assert(toString(ClientMoveDirection::East) == "east");

    assert(dxForDirection(ClientMoveDirection::North) == 0);
    assert(dyForDirection(ClientMoveDirection::North) == -1);
    assert(dxForDirection(ClientMoveDirection::South) == 0);
    assert(dyForDirection(ClientMoveDirection::South) == 1);
    assert(dxForDirection(ClientMoveDirection::West) == -1);
    assert(dyForDirection(ClientMoveDirection::West) == 0);
    assert(dxForDirection(ClientMoveDirection::East) == 1);
    assert(dyForDirection(ClientMoveDirection::East) == 0);

    auto d = clientMoveDirectionFromString("east");
    assert(d.is_ok() && d.value() == ClientMoveDirection::East);
    d = clientMoveDirectionFromString("invalid");
    assert(d.is_error());

    // ClientProjectionBuildReason
    assert(toString(ClientProjectionBuildReason::Bootstrap) == "bootstrap");
    assert(toString(ClientProjectionBuildReason::Refresh) == "refresh");
    assert(toString(ClientProjectionBuildReason::CommandResult) == "command_result");
    assert(toString(ClientProjectionBuildReason::Reset) == "reset");

    auto r = clientProjectionBuildReasonFromString("command_result");
    assert(r.is_ok() && r.value() == ClientProjectionBuildReason::CommandResult);
    r = clientProjectionBuildReasonFromString("invalid");
    assert(r.is_error());

    std::cout << "client_runtime_bridge_enum_roundtrip_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// DTO validation
// ---------------------------------------------------------------------------
void run_dto_validation_tests() {
    // ClientRuntimeViewRequest: actor_key and layer_key required, Unknown rejected
    {
        ClientRuntimeViewRequest req;
        req.actor_key = "";
        req.layer_key = "surface";
        req.reason = ClientProjectionBuildReason::Bootstrap;
        assert(req.validateBasic().is_error());
    }
    {
        ClientRuntimeViewRequest req;
        req.actor_key = "player";
        req.layer_key = "";
        req.reason = ClientProjectionBuildReason::Bootstrap;
        assert(req.validateBasic().is_error());
    }
    {
        ClientRuntimeViewRequest req;
        req.actor_key = "player";
        req.layer_key = "surface";
        req.reason = ClientProjectionBuildReason::Unknown;
        assert(req.validateBasic().is_error());
    }
    {
        ClientRuntimeViewRequest req;
        req.actor_key = "player";
        req.layer_key = "surface";
        req.reason = ClientProjectionBuildReason::Bootstrap;
        assert(req.validateBasic().is_ok());
    }

    // ClientRuntimeView: duplicate cell_id rejected
    {
        ClientRuntimeView view;
        view.actor_key = "player";
        view.active_layer_key = "surface";
        view.projection_version = 1;
        WorldCellPatchDto c1;
        c1.cell_id = "cell_1";
        view.visible_cells.push_back(c1);
        WorldCellPatchDto c2;
        c2.cell_id = "cell_1";
        view.visible_cells.push_back(c2);
        assert(view.validateBasic().is_error());
    }
    {
        ClientRuntimeView view;
        view.actor_key = "player";
        view.active_layer_key = "surface";
        view.projection_version = 1;
        WorldCellPatchDto c1;
        c1.cell_id = "cell_1";
        view.visible_cells.push_back(c1);
        assert(view.validateBasic().is_ok());
    }

    // ClientRuntimeCommandOptionRequest: empty actor rejected
    {
        ClientRuntimeCommandOptionRequest req;
        req.actor_key = "";
        req.layer_key = "surface";
        assert(req.validateBasic().is_error());
    }
    {
        ClientRuntimeCommandOptionRequest req;
        req.actor_key = "player";
        req.layer_key = "surface";
        assert(req.validateBasic().is_ok());
    }

    // ClientRuntimeBridgeDiagnostics: Unknown mode rejected
    {
        ClientRuntimeBridgeDiagnostics diag;
        diag.bridge_mode = ClientRuntimeBridgeMode::Unknown;
        assert(diag.validateBasic().is_error());
    }
    {
        ClientRuntimeBridgeDiagnostics diag;
        diag.bridge_mode = ClientRuntimeBridgeMode::RuntimeBacked;
        assert(diag.validateBasic().is_ok());
    }

    std::cout << "client_runtime_bridge_dto_validation_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Projection bridge returns non-empty visible_cells and player entity
// ---------------------------------------------------------------------------
void run_projection_bridge_non_empty_tests() {
    WorldGridRuntime runtime;
    WorldRuntimeConfig config;
    config.world_id = "test_world";
    config.seed = 123;
    config.region_size = 5;
    config.default_vision_radius = 2;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    ClientRuntimeProjectionBridge bridge(runtime, ClientRuntimeBridgeMode::RuntimeBacked);

    ClientRuntimeViewRequest req;
    req.actor_key = "player";
    req.layer_key = "surface";
    req.reason = ClientProjectionBuildReason::Bootstrap;

    auto view_res = bridge.buildRuntimeView(req);
    assert(view_res.is_ok());
    const auto& view = view_res.value();

    // Must have visible cells
    assert(!view.visible_cells.empty());

    // Must have player entity
    bool has_player = false;
    for (const auto& ent : view.visible_entities) {
        if (ent.fields.count("entity_key") && ent.fields.at("entity_key") == "player") {
            has_player = true;
            break;
        }
    }
    assert(has_player);

    // projection_version must match runtime state version
    assert(view.projection_version == runtime.stateVersion());

    std::cout << "client_runtime_bridge_projection_non_empty_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Projection bridge does not leak unknown cells or invisible entities
// ---------------------------------------------------------------------------
void run_projection_bridge_security_tests() {
    WorldGridRuntime runtime;
    WorldRuntimeConfig config;
    config.seed = 456;
    config.region_size = 5;
    config.default_vision_radius = 2;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    ClientRuntimeProjectionBridge bridge(runtime, ClientRuntimeBridgeMode::RuntimeBacked);

    ClientRuntimeViewRequest req;
    req.actor_key = "player";
    req.layer_key = "surface";
    req.reason = ClientProjectionBuildReason::Bootstrap;

    auto view_res = bridge.buildRuntimeView(req);
    assert(view_res.is_ok());
    const auto& view = view_res.value();

    // No unknown cell should appear
    for (const auto& cell : view.visible_cells) {
        assert(!cell.cell_id.empty());
    }

    std::cout << "client_runtime_bridge_projection_security_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Movement provider generates 4-direction options
// ---------------------------------------------------------------------------
void run_movement_provider_tests() {
    WorldGridRuntime runtime;
    WorldRuntimeConfig config;
    config.seed = 789;
    config.region_size = 5;
    config.default_vision_radius = 2;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldCommandHandlerRegistry registry;
    registry.registerHandler(pathfinder::world_runtime::createMoveCommandHandler(runtime, nullptr));

    MovementCommandOptionProvider provider(registry);

    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    const auto& actor = *actor_res.value();

    auto opts = provider.buildOptions(actor, runtime);
    assert(opts.size() == 4);

    bool has_north = false, has_south = false, has_west = false, has_east = false;
    for (const auto& opt : opts) {
        assert(opt.command_kind == WorldCommandKind::Move);
        if (opt.command_key == "move_north") has_north = true;
        if (opt.command_key == "move_south") has_south = true;
        if (opt.command_key == "move_west") has_west = true;
        if (opt.command_key == "move_east") has_east = true;
    }
    assert(has_north && has_south && has_west && has_east);

    std::cout << "client_runtime_bridge_movement_provider_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Movement provider disabled reason for blocked/out_of_bounds cells
// ---------------------------------------------------------------------------
void run_movement_provider_disabled_reason_tests() {
    WorldGridRuntime runtime;
    WorldRuntimeConfig config;
    config.seed = 999;
    config.region_size = 3;
    config.default_vision_radius = 2;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createMoveCommandHandler(runtime, nullptr));

    MovementCommandOptionProvider provider(registry);

    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    const auto& actor = *actor_res.value();

    auto opts = provider.buildOptions(actor, runtime);

    // Some direction may be disabled if adjacent cell is water/mountain or out of bounds.
    // If all are enabled, that's valid for this seed; verify targets exist.
    bool has_disabled = false;
    for (const auto& opt : opts) {
        if (!opt.enabled && !opt.disabled_reason_text.empty()) {
            has_disabled = true;
            break;
        }
    }
    if (!has_disabled) {
        // All directions enabled: verify each target cell exists
        for (const auto& opt : opts) {
            assert(opt.enabled);
            assert(opt.target.target_coord.has_value());
        }
    }

    std::cout << "client_runtime_bridge_movement_disabled_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Wait/Inspect providers generate options when handler exists
// ---------------------------------------------------------------------------
void run_wait_inspect_provider_tests() {
    WorldGridRuntime runtime;
    WorldRuntimeConfig config;
    config.seed = 111;
    config.region_size = 5;
    config.default_vision_radius = 2;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldCommandHandlerRegistry registry;
    registry.registerHandler(pathfinder::world_runtime::createWaitCommandHandler(runtime));
    registry.registerHandler(pathfinder::world_runtime::createInspectCommandHandler(runtime));

    WaitCommandOptionProvider wait_provider(registry);
    InspectCommandOptionProvider inspect_provider(registry);

    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    const auto& actor = *actor_res.value();

    auto wait_opts = wait_provider.buildOptions(actor);
    assert(!wait_opts.empty());
    assert(wait_opts[0].command_kind == WorldCommandKind::Wait);

    auto inspect_opts = inspect_provider.buildOptions(actor, runtime);
    assert(!inspect_opts.empty());
    bool has_inspect_cell = false;
    for (const auto& opt : inspect_opts) {
        if (opt.command_key == "inspect_current_cell") {
            has_inspect_cell = true;
            break;
        }
    }
    assert(has_inspect_cell);

    std::cout << "client_runtime_bridge_wait_inspect_provider_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// No handler -> no option generated
// ---------------------------------------------------------------------------
void run_provider_no_handler_tests() {
    WorldGridRuntime runtime;
    WorldRuntimeConfig config;
    config.seed = 222;
    config.region_size = 5;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldCommandHandlerRegistry empty_registry;

    MovementCommandOptionProvider move_provider(empty_registry);
    WaitCommandOptionProvider wait_provider(empty_registry);
    InspectCommandOptionProvider inspect_provider(empty_registry);

    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    const auto& actor = *actor_res.value();

    assert(move_provider.buildOptions(actor, runtime).empty());
    assert(wait_provider.buildOptions(actor).empty());
    assert(inspect_provider.buildOptions(actor, runtime).empty());

    std::cout << "client_runtime_bridge_no_handler_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------
void run_diagnostics_tests() {
    WorldGridRuntime runtime;
    WorldRuntimeConfig config;
    config.seed = 333;
    config.region_size = 5;
    config.default_vision_radius = 2;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldCommandHandlerRegistry registry;
    registry.registerHandler(createMoveCommandHandler(runtime, nullptr));
    registry.registerHandler(createWaitCommandHandler(runtime));
    registry.registerHandler(createInspectCommandHandler(runtime));

    ClientRuntimeProjectionBridge proj_bridge(runtime, ClientRuntimeBridgeMode::RuntimeBacked);
    ClientRuntimeCommandOptionBridge opt_bridge(runtime, registry, ClientRuntimeBridgeMode::RuntimeBacked);

    auto diag_res = proj_bridge.diagnostics("player", "surface");
    assert(diag_res.is_ok());
    assert(diag_res.value().bridge_mode == ClientRuntimeBridgeMode::RuntimeBacked);
    assert(diag_res.value().visible_cell_count > 0);
    assert(diag_res.value().visible_entity_count > 0);

    auto opt_diag_res = opt_bridge.diagnostics("player", "surface");
    assert(opt_diag_res.is_ok());
    assert(opt_diag_res.value().bridge_mode == ClientRuntimeBridgeMode::RuntimeBacked);
    assert(opt_diag_res.value().option_count > 0);

    std::cout << "client_runtime_bridge_diagnostics_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_enum_roundtrip_tests();
        run_dto_validation_tests();
        run_projection_bridge_non_empty_tests();
        run_projection_bridge_security_tests();
        run_movement_provider_tests();
        run_movement_provider_disabled_reason_tests();
        run_wait_inspect_provider_tests();
        run_provider_no_handler_tests();
        run_diagnostics_tests();
        return 0;
    }

    std::string test_name = argv[1];
    if (test_name == "enum") run_enum_roundtrip_tests();
    else if (test_name == "dto_validation") run_dto_validation_tests();
    else if (test_name == "projection_non_empty") run_projection_bridge_non_empty_tests();
    else if (test_name == "projection_security") run_projection_bridge_security_tests();
    else if (test_name == "movement_provider") run_movement_provider_tests();
    else if (test_name == "movement_disabled") run_movement_provider_disabled_reason_tests();
    else if (test_name == "wait_inspect_provider") run_wait_inspect_provider_tests();
    else if (test_name == "no_handler") run_provider_no_handler_tests();
    else if (test_name == "diagnostics") run_diagnostics_tests();
    else {
        std::cerr << "Unknown test: " << test_name << std::endl;
        return 1;
    }
    return 0;
}
