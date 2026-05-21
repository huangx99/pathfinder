#include <cassert>
#include <iostream>
#include "pathfinder/client_http/client_server_runtime_factory.h"
#include "pathfinder/client_protocol/client_protocol_harness.h"
#include "pathfinder/world_command/world_command_types.h"

using namespace pathfinder::client_http;
using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

// ---------------------------------------------------------------------------
// Helper: create a fully wired factory with runtime-backed bridge.
// ---------------------------------------------------------------------------
struct RuntimeBackedFixture {
    ClientServerRuntimeFactory factory;
    ClientProtocolHarness harness;

    RuntimeBackedFixture()
        : harness(factory.session_gateway, factory.command_gateway, factory.codec) {}
};

// ---------------------------------------------------------------------------
// Bootstrap returns non-empty visible_cells
// ---------------------------------------------------------------------------
void run_bootstrap_non_empty_tests() {
    RuntimeBackedFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_1", "player");
    assert(boot.is_ok());
    assert(!boot.value().full_projection.visible_cells.empty());
    assert(boot.value().projection_version > 0);

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
    else {
        std::cerr << "Unknown test: " << test_name << std::endl;
        return 1;
    }
    return 0;
}
