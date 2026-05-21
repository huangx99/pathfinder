#include <cassert>
#include <iostream>
#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/client_protocol/client_protocol_codec.h"
#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/client_protocol/client_command_gateway.h"
#include "pathfinder/client_protocol/client_projection_adapter.h"
#include "pathfinder/client_protocol/client_patch_contract.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/client_protocol/client_protocol_harness.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"

using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

// Helper to create a wired harness.
struct HarnessFixture {
    ClientProjectionAdapter projection_adapter;
    ClientPatchContract patch_contract;
    ClientAvailableCommandAdapter available_command_adapter;
    ClientSessionGateway session_gateway;
    WorldCommandHandlerRegistry registry;
    WorldCommandDispatcher dispatcher;
    WorldCommandPipeline pipeline;
    ClientCommandGateway command_gateway;
    ClientProtocolCodec codec;
    ClientProtocolHarness harness;

    HarnessFixture()
        : session_gateway(projection_adapter, available_command_adapter)
        , dispatcher(registry)
        , pipeline(dispatcher)
        , command_gateway(session_gateway, pipeline, patch_contract, available_command_adapter)
        , harness(session_gateway, command_gateway, codec) {
        // Register minimal handlers for protocol testing.
        registry.registerHandler(createNoopCommandHandler());
        registry.registerHandler(createWaitCommandHandler());
        registry.registerHandler(createInspectCommandHandler());
        registry.registerHandler(createSystemTickCommandHandler());
        registry.registerHandler(createGenerateWorldCommandHandler());
    }
};

// ---------------------------------------------------------------------------
// Minimum protocol loop: bootstrap -> command -> patch -> refresh
// ---------------------------------------------------------------------------
void run_bootstrap_command_patch_refresh_tests() {
    HarnessFixture f;

    // 1. Bootstrap
    auto boot = f.harness.fakeBootstrap("client_1", "session_1", "player");
    assert(boot.is_ok());
    assert(boot.value().session_id == "session_1");
    assert(boot.value().projection_version == 1);
    assert(!boot.value().available_commands.empty());

    // Find Wait option
    std::string wait_option_id;
    for (const auto& opt : boot.value().available_commands) {
        if (opt.command_kind == WorldCommandKind::Wait) {
            wait_option_id = opt.option_id;
            break;
        }
    }
    assert(!wait_option_id.empty());

    // 2. Submit Wait option
    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_1", "client_1", 1, boot.value().projection_version, wait_option_id);
    assert(cmd_resp.is_ok());
    assert(cmd_resp.value().result.result_kind == WorldCommandResultKind::Succeeded);
    assert(cmd_resp.value().new_projection_version > cmd_resp.value().base_projection_version);
    assert(!cmd_resp.value().requires_full_refresh);

    // 3. Refresh to confirm version
    auto refresh = f.harness.fakeRefresh(
        "session_1", "client_1", cmd_resp.value().new_projection_version);
    assert(refresh.is_ok());
    assert(refresh.value().projection_version == cmd_resp.value().new_projection_version);

    std::cout << "client_protocol_bootstrap_command_patch_refresh_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Failed command returns safe response
// ---------------------------------------------------------------------------
void run_failed_command_safe_response_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_2", "player");
    assert(boot.is_ok());

    // Submit unknown option_id -> blocked, safe response
    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_2", "client_1", 1, boot.value().projection_version, "opt_expired_xyz");
    assert(cmd_resp.is_ok());
    assert(cmd_resp.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(!cmd_resp.value().result.failure_reason_keys.empty());
    // Must still return available_commands
    assert(!cmd_resp.value().available_commands.empty());
    // Must not crash or leak hidden truth

    std::cout << "client_protocol_failed_command_safe_response_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Stale projection version requires full refresh
// ---------------------------------------------------------------------------
void run_stale_projection_requires_refresh_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_3", "player");
    assert(boot.is_ok());

    // Submit command with stale version (0 instead of 1)
    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_3", "client_1", 1, 0, "opt_wait_player");
    assert(cmd_resp.is_ok());
    // Version mismatch should trigger full refresh flag
    assert(cmd_resp.value().requires_full_refresh);
    assert(cmd_resp.value().projection_patch.requires_full_refresh);

    std::cout << "client_protocol_stale_projection_requires_refresh_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Expired option is blocked
// ---------------------------------------------------------------------------
void run_expired_option_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_4", "player");
    assert(boot.is_ok());

    // Submit a malformed option_id
    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_4", "client_1", 1, boot.value().projection_version, "opt_unknown_12345");
    assert(cmd_resp.is_ok());
    assert(cmd_resp.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(!cmd_resp.value().result.failure_reason_keys.empty());

    std::cout << "client_protocol_expired_option_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Codec roundtrip through harness
// ---------------------------------------------------------------------------
void run_codec_roundtrip_through_harness_tests() {
    HarnessFixture f;

    ClientBootstrapRequest req;
    req.client_id = "client_1";
    req.session_id = "session_5";
    req.requested_actor_key = "player";

    auto resp = f.harness.roundtripBootstrap(req);
    assert(resp.is_ok());
    assert(resp.value().session_id == "session_5");
    assert(resp.value().projection_version == 1);

    std::cout << "client_protocol_codec_roundtrip_through_harness_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Reset creates new session state
// ---------------------------------------------------------------------------
void run_reset_creates_new_session_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_6", "player");
    assert(boot.is_ok());
    uint64_t old_version = boot.value().projection_version;

    // Execute a command to bump version
    auto cmd = f.harness.fakeSubmitOption(
        "session_6", "client_1", 1, old_version, "opt_wait_player");
    assert(cmd.is_ok());
    assert(cmd.value().new_projection_version > old_version);

    // Reset
    auto reset = f.harness.fakeReset("session_6", "client_1");
    assert(reset.is_ok());
    assert(reset.value().session_id == "session_6");
    // After reset, projection version should be back to initial
    assert(reset.value().bootstrap.projection_version == 1);

    std::cout << "client_protocol_reset_creates_new_session_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Available commands only from backend
// ---------------------------------------------------------------------------
void run_available_commands_only_from_backend_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_7", "player");
    assert(boot.is_ok());

    // All available commands should have valid option_ids
    for (const auto& opt : boot.value().available_commands) {
        assert(!opt.option_id.empty());
        assert(!opt.command_key.empty());
        assert(opt.command_kind != WorldCommandKind::Unknown);
    }

    std::cout << "client_protocol_available_commands_only_from_backend_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Command response completeness
// ---------------------------------------------------------------------------
void run_command_response_completeness_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_8", "player");
    assert(boot.is_ok());

    auto cmd = f.harness.fakeSubmitOption(
        "session_8", "client_1", 1, boot.value().projection_version, "opt_wait_player");
    assert(cmd.is_ok());

    // Response must contain result
    assert(cmd.value().result.result_kind != WorldCommandResultKind::Unknown);
    // Response must contain patch with version progression
    assert(cmd.value().projection_patch.new_projection_version >= cmd.value().projection_patch.base_projection_version);
    // Response must contain available_commands
    assert(!cmd.value().available_commands.empty());
    // Response must be valid
    assert(cmd.value().validateBasic().is_ok());

    std::cout << "client_protocol_command_response_completeness_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Forged actor key is ignored; session actor is authority
// ---------------------------------------------------------------------------
void run_forged_actor_ignored_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_forged_actor", "player");
    assert(boot.is_ok());

    // Submit with a forged selected_actor_key in selection_context.
    // The gateway must ignore it and use session actor.
    ClientCommandRequest req;
    req.session_id = "session_forged_actor";
    req.client_id = "client_1";
    req.client_sequence = 1;
    req.known_projection_version = boot.value().projection_version;
    req.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd;
    cmd.command_id = "cmd_1";
    cmd.command_key = "wait";
    cmd.command_kind = WorldCommandKind::Wait;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player"; // must match session actor
    req.command = cmd;
    req.selection_context.selected_actor_key = "wolf"; // forged, must be ignored

    auto resp = f.command_gateway.handleCommand(req);
    assert(resp.is_ok());
    // Should succeed because session actor is "player", not "wolf".
    assert(resp.value().result.result_kind == WorldCommandResultKind::Succeeded);

    std::cout << "client_protocol_forged_actor_ignored_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// WorldCommandDto with wrong actor_key is blocked
// ---------------------------------------------------------------------------
void run_world_command_dto_wrong_actor_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_dto_actor", "player");
    assert(boot.is_ok());

    ClientCommandRequest req;
    req.session_id = "session_dto_actor";
    req.client_id = "client_1";
    req.client_sequence = 1;
    req.known_projection_version = boot.value().projection_version;
    req.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd;
    cmd.command_id = "cmd_1";
    cmd.command_key = "wait";
    cmd.command_kind = WorldCommandKind::Wait;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "wolf"; // does not match session actor
    req.command = cmd;

    auto resp = f.command_gateway.handleCommand(req);
    assert(resp.is_ok());
    assert(resp.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(!resp.value().result.failure_reason_keys.empty());

    std::cout << "client_protocol_world_command_dto_wrong_actor_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Stale projection version does not execute command or advance version
// ---------------------------------------------------------------------------
void run_stale_projection_no_execution_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_stale_no_exec", "player");
    assert(boot.is_ok());
    uint64_t version_before = boot.value().projection_version;

    // Submit with stale known_projection_version (0 instead of actual).
    auto resp = f.harness.fakeSubmitOption(
        "session_stale_no_exec", "client_1", 1, 0, "opt_wait_player");
    assert(resp.is_ok());

    // Must be blocked, not executed.
    assert(resp.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(resp.value().requires_full_refresh);
    // Projection version must not advance.
    assert(resp.value().new_projection_version == version_before);

    // Verify session version did not change.
    auto refresh = f.harness.fakeRefresh(
        "session_stale_no_exec", "client_1", version_before);
    assert(refresh.is_ok());
    assert(refresh.value().projection_version == version_before);

    std::cout << "client_protocol_stale_projection_no_execution_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Forged option_id is blocked by snapshot authority
// ---------------------------------------------------------------------------
void run_forged_option_id_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_forged_opt", "player");
    assert(boot.is_ok());

    // Submit a forged option_id that looks valid but is not in snapshot.
    auto resp = f.harness.fakeSubmitOption(
        "session_forged_opt", "client_1", 1, boot.value().projection_version, "opt_wait_wolf");
    assert(resp.is_ok());
    assert(resp.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(!resp.value().result.failure_reason_keys.empty());

    std::cout << "client_protocol_forged_option_id_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Old option_id after version advance is blocked
// ---------------------------------------------------------------------------
void run_old_option_after_version_advance_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_old_opt", "player");
    assert(boot.is_ok());
    uint64_t v1 = boot.value().projection_version;

    // Execute a command to advance version.
    auto cmd1 = f.harness.fakeSubmitOption(
        "session_old_opt", "client_1", 1, v1, "opt_wait_player");
    assert(cmd1.is_ok());
    assert(cmd1.value().result.result_kind == WorldCommandResultKind::Succeeded);
    uint64_t v2 = cmd1.value().new_projection_version;
    assert(v2 > v1);

    // Submit with old known_projection_version and old option_id.
    // Must be blocked due to version mismatch before option authority is even checked.
    auto cmd2 = f.harness.fakeSubmitOption(
        "session_old_opt", "client_1", 2, v1, "opt_wait_player");
    assert(cmd2.is_ok());
    assert(cmd2.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(cmd2.value().requires_full_refresh);

    std::cout << "client_protocol_old_option_after_version_advance_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// WorldCommandDto with illegal source is blocked
// ---------------------------------------------------------------------------
void run_world_command_dto_illegal_source_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_dto_source", "player");
    assert(boot.is_ok());

    ClientCommandRequest req;
    req.session_id = "session_dto_source";
    req.client_id = "client_1";
    req.client_sequence = 1;
    req.known_projection_version = boot.value().projection_version;
    req.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd;
    cmd.command_id = "cmd_1";
    cmd.command_key = "wait";
    cmd.command_kind = WorldCommandKind::Wait;
    cmd.source = WorldCommandSource::AgentDecision; // illegal for client
    cmd.actor_key = "player";
    req.command = cmd;

    auto resp = f.command_gateway.handleCommand(req);
    assert(resp.is_ok());
    assert(resp.value().result.result_kind == WorldCommandResultKind::Blocked);

    std::cout << "client_protocol_world_command_dto_illegal_source_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Command response available_commands is persisted to session snapshot
// ---------------------------------------------------------------------------
void run_command_updates_snapshot_authority_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_snapshot", "player");
    assert(boot.is_ok());
    uint64_t v1 = boot.value().projection_version;

    // Execute an OptionId command successfully.
    auto cmd1 = f.harness.fakeSubmitOption(
        "session_snapshot", "client_1", 1, v1, "opt_wait_player");
    assert(cmd1.is_ok());
    assert(cmd1.value().result.result_kind == WorldCommandResultKind::Succeeded);
    uint64_t v2 = cmd1.value().new_projection_version;

    // Verify snapshot was persisted and matches response available_commands.
    auto snapshot = f.session_gateway.getAvailableCommands("session_snapshot");
    assert(snapshot.is_ok());
    assert(!snapshot.value().empty());
    assert(snapshot.value().size() == cmd1.value().available_commands.size());
    for (size_t i = 0; i < snapshot.value().size(); ++i) {
        assert(snapshot.value()[i].option_id == cmd1.value().available_commands[i].option_id);
        assert(snapshot.value()[i].command_key == cmd1.value().available_commands[i].command_key);
        assert(snapshot.value()[i].command_kind == cmd1.value().available_commands[i].command_kind);
    }

    // The persisted snapshot should allow further OptionId commands.
    auto cmd2 = f.harness.fakeSubmitOption(
        "session_snapshot", "client_1", 2, v2, "opt_wait_player");
    assert(cmd2.is_ok());
    assert(cmd2.value().result.result_kind == WorldCommandResultKind::Succeeded);

    std::cout << "client_protocol_command_updates_snapshot_authority_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// ClientId mismatch is blocked
// ---------------------------------------------------------------------------
void run_client_id_mismatch_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_client_id", "player");
    assert(boot.is_ok());

    // Submit with wrong client_id.
    auto cmd_resp = f.harness.fakeSubmitOption(
        "session_client_id", "client_2", 1, boot.value().projection_version, "opt_wait_player");
    assert(cmd_resp.is_ok());
    assert(cmd_resp.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(!cmd_resp.value().result.failure_reason_keys.empty());

    // Refresh with wrong client_id should also fail.
    auto refresh = f.harness.fakeRefresh(
        "session_client_id", "client_2", boot.value().projection_version);
    assert(refresh.is_error());

    std::cout << "client_protocol_client_id_mismatch_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Blocked response can be encoded as a valid wire response
// ---------------------------------------------------------------------------
void run_blocked_response_codec_roundtrip_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_codec_block", "player");
    assert(boot.is_ok());

    // 1. Stale version blocked response must be valid and encodable.
    ClientCommandRequest req1;
    req1.session_id = "session_codec_block";
    req1.client_id = "client_1";
    req1.client_sequence = 1;
    req1.known_projection_version = 0; // stale
    req1.submit_mode = ClientSubmitMode::OptionId;
    req1.option_id = "opt_wait_player";
    auto resp1 = f.command_gateway.handleCommand(req1);
    assert(resp1.is_ok());
    assert(resp1.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(resp1.value().validateBasic().is_ok());
    auto enc1 = f.codec.encodeCommandResponse(resp1.value());
    assert(enc1.is_ok());
    auto dec1 = f.codec.decodeCommandResponse(enc1.value());
    assert(dec1.is_ok());
    assert(dec1.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(dec1.value().result.command_id == resp1.value().result.command_id);

    // 2. Expired option blocked response must be valid and encodable.
    ClientCommandRequest req2;
    req2.session_id = "session_codec_block";
    req2.client_id = "client_1";
    req2.client_sequence = 2;
    req2.known_projection_version = boot.value().projection_version;
    req2.submit_mode = ClientSubmitMode::OptionId;
    req2.option_id = "opt_expired_xyz";
    auto resp2 = f.command_gateway.handleCommand(req2);
    assert(resp2.is_ok());
    assert(resp2.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(resp2.value().validateBasic().is_ok());
    auto enc2 = f.codec.encodeCommandResponse(resp2.value());
    assert(enc2.is_ok());

    // 3. ClientId mismatch blocked response must be valid and encodable.
    ClientCommandRequest req3;
    req3.session_id = "session_codec_block";
    req3.client_id = "client_2"; // wrong
    req3.client_sequence = 3;
    req3.known_projection_version = boot.value().projection_version;
    req3.submit_mode = ClientSubmitMode::OptionId;
    req3.option_id = "opt_wait_player";
    auto resp3 = f.command_gateway.handleCommand(req3);
    assert(resp3.is_ok());
    assert(resp3.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(resp3.value().validateBasic().is_ok());
    auto enc3 = f.codec.encodeCommandResponse(resp3.value());
    assert(enc3.is_ok());

    std::cout << "client_protocol_blocked_response_codec_roundtrip_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Multiple sessions do not crosstalk projection versions
// ---------------------------------------------------------------------------
void run_multi_session_version_isolation_tests() {
    HarnessFixture f;

    // Bootstrap two sessions sharing the same pipeline.
    auto boot1 = f.harness.fakeBootstrap("client_1", "session_s1", "player");
    assert(boot1.is_ok());
    uint64_t v1_s1 = boot1.value().projection_version;

    auto boot2 = f.harness.fakeBootstrap("client_2", "session_s2", "player");
    assert(boot2.is_ok());
    uint64_t v1_s2 = boot2.value().projection_version;

    // Execute command on session_1 to advance version.
    auto cmd1 = f.harness.fakeSubmitOption(
        "session_s1", "client_1", 1, v1_s1, "opt_wait_player");
    assert(cmd1.is_ok());
    assert(cmd1.value().result.result_kind == WorldCommandResultKind::Succeeded);
    assert(cmd1.value().new_projection_version > v1_s1);
    uint64_t v2_s1 = cmd1.value().new_projection_version;

    // Session_2 submits with its own known version.
    // Must succeed without requiring full refresh, because pipeline version
    // is synced to session_2's current version before execution.
    auto cmd2 = f.harness.fakeSubmitOption(
        "session_s2", "client_2", 1, v1_s2, "opt_wait_player");
    assert(cmd2.is_ok());
    assert(cmd2.value().result.result_kind == WorldCommandResultKind::Succeeded);
    // Must NOT require full refresh.
    assert(!cmd2.value().requires_full_refresh);
    // Patch base must match known version.
    assert(cmd2.value().projection_patch.base_projection_version == v1_s2);
    assert(cmd2.value().new_projection_version > v1_s2);

    // Session_1 can continue with its updated version.
    auto cmd3 = f.harness.fakeSubmitOption(
        "session_s1", "client_1", 2, v2_s1, "opt_wait_player");
    assert(cmd3.is_ok());
    assert(cmd3.value().result.result_kind == WorldCommandResultKind::Succeeded);
    assert(!cmd3.value().requires_full_refresh);

    std::cout << "client_protocol_multi_session_version_isolation_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Unsafe parameters in WorldCommandDto are blocked
// ---------------------------------------------------------------------------
void run_unsafe_parameters_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_unsafe", "player");
    assert(boot.is_ok());

    // Helper to submit a WorldCommandDto with custom fields.
    auto submitDto = [&](const WorldCommandDto& cmd) -> Result<ClientCommandResponse> {
        ClientCommandRequest req;
        req.session_id = "session_unsafe";
        req.client_id = "client_1";
        req.client_sequence = 1;
        req.known_projection_version = boot.value().projection_version;
        req.submit_mode = ClientSubmitMode::WorldCommandDto;
        req.command = cmd;
        return f.command_gateway.handleCommand(req);
    };

    // 1. allow_hidden_debug=true is blocked.
    WorldCommandDto cmd1;
    cmd1.command_id = "cmd_1";
    cmd1.command_key = "wait";
    cmd1.command_kind = WorldCommandKind::Wait;
    cmd1.source = WorldCommandSource::PlayerInput;
    cmd1.actor_key = "player";
    cmd1.context.allow_hidden_debug = true;
    auto resp1 = submitDto(cmd1);
    assert(resp1.is_ok());
    assert(resp1.value().result.result_kind == WorldCommandResultKind::Blocked);

    // 2. force_teach parameter is blocked.
    WorldCommandDto cmd2;
    cmd2.command_id = "cmd_2";
    cmd2.command_key = "wait";
    cmd2.command_kind = WorldCommandKind::Wait;
    cmd2.source = WorldCommandSource::PlayerInput;
    cmd2.actor_key = "player";
    cmd2.parameters["force_teach"] = "true";
    auto resp2 = submitDto(cmd2);
    assert(resp2.is_ok());
    assert(resp2.value().result.result_kind == WorldCommandResultKind::Blocked);

    // 3. recipient_claim_json parameter is blocked.
    WorldCommandDto cmd3;
    cmd3.command_id = "cmd_3";
    cmd3.command_key = "wait";
    cmd3.command_kind = WorldCommandKind::Wait;
    cmd3.source = WorldCommandSource::PlayerInput;
    cmd3.actor_key = "player";
    cmd3.parameters["recipient_claim_json"] = "{}";
    auto resp3 = submitDto(cmd3);
    assert(resp3.is_ok());
    assert(resp3.value().result.result_kind == WorldCommandResultKind::Blocked);

    // 4. Normal command without unsafe fields succeeds.
    WorldCommandDto cmd4;
    cmd4.command_id = "cmd_4";
    cmd4.command_key = "wait";
    cmd4.command_kind = WorldCommandKind::Wait;
    cmd4.source = WorldCommandSource::PlayerInput;
    cmd4.actor_key = "player";
    auto resp4 = submitDto(cmd4);
    assert(resp4.is_ok());
    assert(resp4.value().result.result_kind == WorldCommandResultKind::Succeeded);

    std::cout << "client_protocol_unsafe_parameters_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// WorldCommandDto with unissued command_kind is blocked
// ---------------------------------------------------------------------------
void run_world_command_dto_generate_world_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_dto_gen", "player");
    assert(boot.is_ok());

    // Submit WorldCommandDto with GenerateWorld — not issued in available_commands.
    ClientCommandRequest req;
    req.session_id = "session_dto_gen";
    req.client_id = "client_1";
    req.client_sequence = 1;
    req.known_projection_version = boot.value().projection_version;
    req.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd;
    cmd.command_id = "cmd_1";
    cmd.command_key = "generate_world";
    cmd.command_kind = WorldCommandKind::GenerateWorld;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";
    req.command = cmd;

    auto resp = f.command_gateway.handleCommand(req);
    assert(resp.is_ok());
    assert(resp.value().result.result_kind == WorldCommandResultKind::Blocked);
    assert(!resp.value().result.failure_reason_keys.empty());

    std::cout << "client_protocol_world_command_dto_generate_world_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// WorldCommandDto with command_key mismatch is blocked
// ---------------------------------------------------------------------------
void run_world_command_dto_key_mismatch_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_dto_key", "player");
    assert(boot.is_ok());

    // Submit WorldCommandDto where command_kind=Wait but command_key is wrong.
    ClientCommandRequest req;
    req.session_id = "session_dto_key";
    req.client_id = "client_1";
    req.client_sequence = 1;
    req.known_projection_version = boot.value().projection_version;
    req.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd;
    cmd.command_id = "cmd_1";
    cmd.command_key = "invalid_key";
    cmd.command_kind = WorldCommandKind::Wait;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";
    req.command = cmd;

    auto resp = f.command_gateway.handleCommand(req);
    assert(resp.is_ok());
    assert(resp.value().result.result_kind == WorldCommandResultKind::Blocked);

    std::cout << "client_protocol_world_command_dto_key_mismatch_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// WorldCommandDto with target mismatch is blocked
// ---------------------------------------------------------------------------
void run_world_command_dto_target_mismatch_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_dto_target", "player");
    assert(boot.is_ok());

    // Submit WorldCommandDto where target_kind does not match any issued option.
    // Use Actor target (valid shape) but mismatch against Wait option which has None target.
    ClientCommandRequest req;
    req.session_id = "session_dto_target";
    req.client_id = "client_1";
    req.client_sequence = 1;
    req.known_projection_version = boot.value().projection_version;
    req.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd;
    cmd.command_id = "cmd_1";
    cmd.command_key = "wait";
    cmd.command_kind = WorldCommandKind::Wait;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";
    cmd.target.target_kind = WorldCommandTargetKind::Actor; // mismatched: Wait option has None target
    cmd.target.target_actor_key = "player";
    req.command = cmd;

    auto resp = f.command_gateway.handleCommand(req);
    assert(resp.is_ok());
    assert(resp.value().result.result_kind == WorldCommandResultKind::Blocked);

    std::cout << "client_protocol_world_command_dto_target_mismatch_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// WorldCommandDto with forged target_entity_id is blocked
// ---------------------------------------------------------------------------
void run_world_command_dto_target_entity_forged_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_dto_entity", "player");
    assert(boot.is_ok());

    // Inject a snapshot with an Inspect option targeting a specific entity.
    WorldCommandOptionDto opt;
    opt.option_id = "opt_inspect_issued";
    opt.command_kind = WorldCommandKind::Inspect;
    opt.command_key = "inspect";
    opt.label_text = "Inspect Issued Entity";
    opt.target.target_kind = WorldCommandTargetKind::Entity;
    opt.target.target_entity_id = "issued_entity";
    std::vector<WorldCommandOptionDto> injected_snapshot = {opt};
    f.session_gateway.updateAvailableCommands("session_dto_entity", injected_snapshot);

    // 1. Submit Dto with forged target_entity_id -> blocked.
    ClientCommandRequest req1;
    req1.session_id = "session_dto_entity";
    req1.client_id = "client_1";
    req1.client_sequence = 1;
    req1.known_projection_version = boot.value().projection_version;
    req1.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd1;
    cmd1.command_id = "cmd_1";
    cmd1.command_key = "inspect";
    cmd1.command_kind = WorldCommandKind::Inspect;
    cmd1.source = WorldCommandSource::PlayerInput;
    cmd1.actor_key = "player";
    cmd1.target.target_kind = WorldCommandTargetKind::Entity;
    cmd1.target.target_entity_id = "forged_entity";
    req1.command = cmd1;
    auto resp1 = f.command_gateway.handleCommand(req1);
    assert(resp1.is_ok());
    assert(resp1.value().result.result_kind == WorldCommandResultKind::Blocked);

    // 2. Submit Dto with correct target_entity_id -> succeeds.
    ClientCommandRequest req2;
    req2.session_id = "session_dto_entity";
    req2.client_id = "client_1";
    req2.client_sequence = 2;
    req2.known_projection_version = boot.value().projection_version;
    req2.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd2;
    cmd2.command_id = "cmd_2";
    cmd2.command_key = "inspect";
    cmd2.command_kind = WorldCommandKind::Inspect;
    cmd2.source = WorldCommandSource::PlayerInput;
    cmd2.actor_key = "player";
    cmd2.target.target_kind = WorldCommandTargetKind::Entity;
    cmd2.target.target_entity_id = "issued_entity";
    req2.command = cmd2;
    auto resp2 = f.command_gateway.handleCommand(req2);
    assert(resp2.is_ok());
    assert(resp2.value().result.result_kind == WorldCommandResultKind::Succeeded);

    std::cout << "client_protocol_world_command_dto_target_entity_forged_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// WorldCommandDto with mismatched coordinate target is blocked
// ---------------------------------------------------------------------------
void run_world_command_dto_target_coord_mismatch_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_dto_coord", "player");
    assert(boot.is_ok());

    // Inject a snapshot with a Wait option targeting a specific coordinate.
    WorldCommandOptionDto opt;
    opt.option_id = "opt_wait_coord";
    opt.command_kind = WorldCommandKind::Wait;
    opt.command_key = "wait";
    opt.label_text = "Wait at Coordinate";
    opt.target.target_kind = WorldCommandTargetKind::Coordinate;
    opt.target.target_coord = WorldCoordinateDto{1, 2, "surface"};
    std::vector<WorldCommandOptionDto> injected_snapshot = {opt};
    f.session_gateway.updateAvailableCommands("session_dto_coord", injected_snapshot);

    // 1. Submit Dto with mismatched coordinate -> blocked.
    ClientCommandRequest req1;
    req1.session_id = "session_dto_coord";
    req1.client_id = "client_1";
    req1.client_sequence = 1;
    req1.known_projection_version = boot.value().projection_version;
    req1.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd1;
    cmd1.command_id = "cmd_1";
    cmd1.command_key = "wait";
    cmd1.command_kind = WorldCommandKind::Wait;
    cmd1.source = WorldCommandSource::PlayerInput;
    cmd1.actor_key = "player";
    cmd1.target.target_kind = WorldCommandTargetKind::Coordinate;
    cmd1.target.target_coord = WorldCoordinateDto{3, 4, "surface"};
    req1.command = cmd1;
    auto resp1 = f.command_gateway.handleCommand(req1);
    assert(resp1.is_ok());
    assert(resp1.value().result.result_kind == WorldCommandResultKind::Blocked);

    // 2. Submit Dto with correct coordinate -> succeeds.
    ClientCommandRequest req2;
    req2.session_id = "session_dto_coord";
    req2.client_id = "client_1";
    req2.client_sequence = 2;
    req2.known_projection_version = boot.value().projection_version;
    req2.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd2;
    cmd2.command_id = "cmd_2";
    cmd2.command_key = "wait";
    cmd2.command_kind = WorldCommandKind::Wait;
    cmd2.source = WorldCommandSource::PlayerInput;
    cmd2.actor_key = "player";
    cmd2.target.target_kind = WorldCommandTargetKind::Coordinate;
    cmd2.target.target_coord = WorldCoordinateDto{1, 2, "surface"};
    req2.command = cmd2;
    auto resp2 = f.command_gateway.handleCommand(req2);
    assert(resp2.is_ok());
    assert(resp2.value().result.result_kind == WorldCommandResultKind::Succeeded);

    // 3. Submit Dto with mismatched layer_key -> blocked.
    ClientCommandRequest req3;
    req3.session_id = "session_dto_coord";
    req3.client_id = "client_1";
    req3.client_sequence = 3;
    req3.known_projection_version = boot.value().projection_version;
    req3.submit_mode = ClientSubmitMode::WorldCommandDto;
    WorldCommandDto cmd3;
    cmd3.command_id = "cmd_3";
    cmd3.command_key = "wait";
    cmd3.command_kind = WorldCommandKind::Wait;
    cmd3.source = WorldCommandSource::PlayerInput;
    cmd3.actor_key = "player";
    cmd3.target.target_kind = WorldCommandTargetKind::Coordinate;
    cmd3.target.target_coord = WorldCoordinateDto{1, 2, "underground"};
    req3.command = cmd3;
    auto resp3 = f.command_gateway.handleCommand(req3);
    assert(resp3.is_ok());
    assert(resp3.value().result.result_kind == WorldCommandResultKind::Blocked);

    std::cout << "client_protocol_world_command_dto_target_coord_mismatch_blocked_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// WorldCommandDto with injected parameters is blocked
// ---------------------------------------------------------------------------
void run_world_command_dto_parameters_injected_blocked_tests() {
    HarnessFixture f;

    auto boot = f.harness.fakeBootstrap("client_1", "session_dto_params", "player");
    assert(boot.is_ok());

    // Helper to submit a WorldCommandDto with custom parameters.
    auto submitDto = [&](const WorldCommandDto& cmd) -> Result<ClientCommandResponse> {
        ClientCommandRequest req;
        req.session_id = "session_dto_params";
        req.client_id = "client_1";
        req.client_sequence = 1;
        req.known_projection_version = boot.value().projection_version;
        req.submit_mode = ClientSubmitMode::WorldCommandDto;
        req.command = cmd;
        return f.command_gateway.handleCommand(req);
    };

    // 1. Any non-empty parameters are blocked (tick_delta).
    WorldCommandDto cmd1;
    cmd1.command_id = "cmd_1";
    cmd1.command_key = "wait";
    cmd1.command_kind = WorldCommandKind::Wait;
    cmd1.source = WorldCommandSource::PlayerInput;
    cmd1.actor_key = "player";
    cmd1.parameters["tick_delta"] = "999";
    auto resp1 = submitDto(cmd1);
    assert(resp1.is_ok());
    assert(resp1.value().result.result_kind == WorldCommandResultKind::Blocked);

    // 2. Any non-empty parameters are blocked (quantity).
    WorldCommandDto cmd2;
    cmd2.command_id = "cmd_2";
    cmd2.command_key = "wait";
    cmd2.command_kind = WorldCommandKind::Wait;
    cmd2.source = WorldCommandSource::PlayerInput;
    cmd2.actor_key = "player";
    cmd2.parameters["quantity"] = "999";
    auto resp2 = submitDto(cmd2);
    assert(resp2.is_ok());
    assert(resp2.value().result.result_kind == WorldCommandResultKind::Blocked);

    // 3. Normal command without parameters still succeeds.
    WorldCommandDto cmd3;
    cmd3.command_id = "cmd_3";
    cmd3.command_key = "wait";
    cmd3.command_kind = WorldCommandKind::Wait;
    cmd3.source = WorldCommandSource::PlayerInput;
    cmd3.actor_key = "player";
    auto resp3 = submitDto(cmd3);
    assert(resp3.is_ok());
    assert(resp3.value().result.result_kind == WorldCommandResultKind::Succeeded);

    std::cout << "client_protocol_world_command_dto_parameters_injected_blocked_tests: all passed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: bootstrap_command_patch_refresh, failed_command_safe_response, stale_projection_requires_refresh, expired_option_blocked, codec_roundtrip_through_harness, reset_creates_new_session, available_commands_only_from_backend, command_response_completeness, forged_actor_ignored, world_command_dto_wrong_actor_blocked, stale_projection_no_execution, forged_option_id_blocked, old_option_after_version_advance_blocked, world_command_dto_illegal_source_blocked, command_updates_snapshot_authority, client_id_mismatch_blocked, blocked_response_codec_roundtrip, multi_session_version_isolation, unsafe_parameters_blocked, world_command_dto_generate_world_blocked, world_command_dto_key_mismatch_blocked, world_command_dto_target_mismatch_blocked, world_command_dto_target_entity_forged_blocked, world_command_dto_target_coord_mismatch_blocked, world_command_dto_parameters_injected_blocked" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "bootstrap_command_patch_refresh") {
        run_bootstrap_command_patch_refresh_tests();
    } else if (test_name == "failed_command_safe_response") {
        run_failed_command_safe_response_tests();
    } else if (test_name == "stale_projection_requires_refresh") {
        run_stale_projection_requires_refresh_tests();
    } else if (test_name == "expired_option_blocked") {
        run_expired_option_blocked_tests();
    } else if (test_name == "codec_roundtrip_through_harness") {
        run_codec_roundtrip_through_harness_tests();
    } else if (test_name == "reset_creates_new_session") {
        run_reset_creates_new_session_tests();
    } else if (test_name == "available_commands_only_from_backend") {
        run_available_commands_only_from_backend_tests();
    } else if (test_name == "command_response_completeness") {
        run_command_response_completeness_tests();
    } else if (test_name == "forged_actor_ignored") {
        run_forged_actor_ignored_tests();
    } else if (test_name == "world_command_dto_wrong_actor_blocked") {
        run_world_command_dto_wrong_actor_blocked_tests();
    } else if (test_name == "stale_projection_no_execution") {
        run_stale_projection_no_execution_tests();
    } else if (test_name == "forged_option_id_blocked") {
        run_forged_option_id_blocked_tests();
    } else if (test_name == "old_option_after_version_advance_blocked") {
        run_old_option_after_version_advance_blocked_tests();
    } else if (test_name == "world_command_dto_illegal_source_blocked") {
        run_world_command_dto_illegal_source_blocked_tests();
    } else if (test_name == "command_updates_snapshot_authority") {
        run_command_updates_snapshot_authority_tests();
    } else if (test_name == "client_id_mismatch_blocked") {
        run_client_id_mismatch_blocked_tests();
    } else if (test_name == "blocked_response_codec_roundtrip") {
        run_blocked_response_codec_roundtrip_tests();
    } else if (test_name == "multi_session_version_isolation") {
        run_multi_session_version_isolation_tests();
    } else if (test_name == "unsafe_parameters_blocked") {
        run_unsafe_parameters_blocked_tests();
    } else if (test_name == "world_command_dto_generate_world_blocked") {
        run_world_command_dto_generate_world_blocked_tests();
    } else if (test_name == "world_command_dto_key_mismatch_blocked") {
        run_world_command_dto_key_mismatch_blocked_tests();
    } else if (test_name == "world_command_dto_target_mismatch_blocked") {
        run_world_command_dto_target_mismatch_blocked_tests();
    } else if (test_name == "world_command_dto_target_entity_forged_blocked") {
        run_world_command_dto_target_entity_forged_blocked_tests();
    } else if (test_name == "world_command_dto_target_coord_mismatch_blocked") {
        run_world_command_dto_target_coord_mismatch_blocked_tests();
    } else if (test_name == "world_command_dto_parameters_injected_blocked") {
        run_world_command_dto_parameters_injected_blocked_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
