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

ClientAvailableCommandAdapter::ClientAvailableCommandAdapter() = default;

Result<std::vector<WorldCommandOptionDto>> ClientAvailableCommandAdapter::buildOptions(
    const std::string& actor_key,
    const std::string& /*layer_key*/) const {

    std::vector<WorldCommandOptionDto> options;

    // Wait option
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

    // Inspect option
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

    // Fallback without snapshot (legacy / unit-test only).
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
