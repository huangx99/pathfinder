#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/client_runtime_bridge/client_runtime_bridge_port.h"
#include "pathfinder/foundation/result.h"
#include <memory>
#include <vector>

namespace pathfinder::client_protocol {

// Provides available command options from backend context.
// Does not hard-code content keys or read JSON.
class ClientAvailableCommandAdapter {
public:
    ClientAvailableCommandAdapter();
    explicit ClientAvailableCommandAdapter(
        std::shared_ptr<pathfinder::client_runtime_bridge::IClientRuntimeBridgePort> option_bridge);

    // Build available commands for the given actor context.
    // P56: uses runtime option bridge if injected; otherwise falls back to stub for tests.
    pathfinder::foundation::Result<std::vector<WorldCommandOptionDto>> buildOptions(
        const std::string& actor_key,
        const std::string& layer_key) const;

    // Materialize an option_id into a WorldCommandDto using a specific snapshot.
    // The option_id must exist in the snapshot; otherwise returns failure.
    pathfinder::foundation::Result<WorldCommandDto> materializeOption(
        const std::string& option_id,
        const std::string& actor_key,
        const std::vector<WorldCommandOptionDto>& snapshot) const;

    // Materialize without snapshot (for backward-compat / unit tests only).
    // Real client protocol should always use the snapshot overload.
    pathfinder::foundation::Result<WorldCommandDto> materializeOption(
        const std::string& option_id,
        const std::string& actor_key) const;

private:
    std::shared_ptr<pathfinder::client_runtime_bridge::IClientRuntimeBridgePort> option_bridge_;

    pathfinder::foundation::Result<std::vector<WorldCommandOptionDto>> buildFromBridge(
        const std::string& actor_key,
        const std::string& layer_key) const;
};

} // namespace pathfinder::client_protocol
