#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/foundation/error.h"

#include <atomic>
#include <chrono>

namespace pathfinder::client_protocol {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace {
    std::atomic<uint64_t> g_command_counter{0};
    std::string makeCommandId(const std::string& prefix) {
        uint64_t n = ++g_command_counter;
        uint64_t ts = static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        return prefix + "_" + std::to_string(ts) + "_" + std::to_string(n);
    }
}

// Test-only compatibility constructor. Production must inject option_bridge so
// available_commands are generated from runtime providers rather than the fallback.
ClientAvailableCommandAdapter::ClientAvailableCommandAdapter() = default;

ClientAvailableCommandAdapter::ClientAvailableCommandAdapter(
    std::shared_ptr<pathfinder::client_runtime_bridge::IClientRuntimeBridgePort> option_bridge)
    : option_bridge_(std::move(option_bridge)) {}

Result<std::vector<WorldCommandOptionDto>> ClientAvailableCommandAdapter::buildFromBridge(
    const std::string& actor_key,
    const std::string& layer_key) const {

    pathfinder::client_runtime_bridge::ClientRuntimeCommandOptionRequest req;
    req.actor_key = actor_key;
    req.layer_key = layer_key.empty() ? "surface" : layer_key;
    // Use all providers by default (empty provider_kinds means all)

    auto opts_res = option_bridge_->buildRuntimeOptions(req);
    if (opts_res.is_error()) {
        return Result<std::vector<WorldCommandOptionDto>>::fail(opts_res.errors());
    }

    return Result<std::vector<WorldCommandOptionDto>>::ok(std::move(opts_res.value()));
}

Result<std::vector<WorldCommandOptionDto>> ClientAvailableCommandAdapter::buildOptions(
    const std::string& actor_key,
    const std::string& layer_key) const {

    if (option_bridge_) {
        return buildFromBridge(actor_key, layer_key);
    }

    // P53 test fallback only: returns a minimal Wait/Inspect set for protocol tests.
    // Production must not rely on this branch. If a playable command is needed, add
    // a runtime-backed provider/handler and wire it through ClientRuntimeHostFactory.
    std::vector<WorldCommandOptionDto> options;

    {
        WorldCommandOptionDto opt;
        opt.option_id = "opt_wait_" + actor_key;
        opt.command_kind = WorldCommandKind::Wait;
        opt.command_key = "wait";
        opt.label_text = "Wait";
        opt.enabled = true;
        opt.priority = 0;
        options.push_back(std::move(opt));
    }

    {
        WorldCommandOptionDto opt;
        opt.option_id = "opt_inspect_" + actor_key;
        opt.command_kind = WorldCommandKind::Inspect;
        opt.command_key = "inspect";
        opt.label_text = "Inspect";
        opt.enabled = true;
        opt.priority = 1;
        options.push_back(std::move(opt));
    }

    return Result<std::vector<WorldCommandOptionDto>>::ok(std::move(options));
}


Result<uint64_t> ClientAvailableCommandAdapter::runtimeStateVersion(
    const std::string& actor_key,
    const std::string& layer_key) const {

    if (!option_bridge_) {
        return Result<uint64_t>::ok(0);
    }

    auto diag_res = option_bridge_->diagnostics(
        actor_key, layer_key.empty() ? "surface" : layer_key);
    if (diag_res.is_error()) {
        return Result<uint64_t>::fail(diag_res.errors());
    }

    return Result<uint64_t>::ok(diag_res.value().runtime_state_version);
}

Result<WorldCommandDto> ClientAvailableCommandAdapter::materializeOption(
    const std::string& option_id,
    const std::string& actor_key,
    const std::vector<WorldCommandOptionDto>& snapshot) const {

    // Authority: option_id must exist in the provided snapshot.
    const WorldCommandOptionDto* matched = nullptr;
    for (const auto& opt : snapshot) {
        if (opt.option_id == option_id) {
            matched = &opt;
            break;
        }
    }

    if (!matched) {
        return Result<WorldCommandDto>::fail(
            makeError(ErrorCode::command_unknown_type,
                "Unknown or expired option_id in snapshot: " + option_id)
        );
    }

    // Use the matched option's properties to build the command.
    WorldCommandDto cmd;
    cmd.command_id = makeCommandId("cmd");
    cmd.command_kind = matched->command_kind;
    cmd.command_key = matched->command_key;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = actor_key;
    cmd.target = matched->target;
    return Result<WorldCommandDto>::ok(std::move(cmd));
}


Result<WorldCommandDto> ClientAvailableCommandAdapter::materializeOption(
    const std::string& option_id,
    const std::string& actor_key) const {

    // Legacy fallback without snapshot: unit-test/backward-compat only.
    // ClientCommandGateway uses the snapshot overload; do not call this from
    // production client request handling or it would bypass session option authority.
    if (option_id.find("opt_wait_") == 0) {
        WorldCommandDto cmd;
        cmd.command_id = makeCommandId("cmd");
        cmd.command_kind = WorldCommandKind::Wait;
        cmd.command_key = "wait";
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = actor_key;
        return Result<WorldCommandDto>::ok(std::move(cmd));
    }

    if (option_id.find("opt_inspect_") == 0) {
        WorldCommandDto cmd;
        cmd.command_id = makeCommandId("cmd");
        cmd.command_kind = WorldCommandKind::Inspect;
        cmd.command_key = "inspect";
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = actor_key;
        cmd.target.target_kind = WorldCommandTargetKind::None;
        return Result<WorldCommandDto>::ok(std::move(cmd));
    }

    return Result<WorldCommandDto>::fail(
        makeError(ErrorCode::command_unknown_type,
            "Unknown or expired option_id: " + option_id)
    );
}

} // namespace pathfinder::client_protocol
