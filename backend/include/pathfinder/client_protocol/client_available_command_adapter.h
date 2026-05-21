#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::client_protocol {

// Provides available command options from backend context.
// Does not hard-code content keys or read JSON.
class ClientAvailableCommandAdapter {
public:
    ClientAvailableCommandAdapter();

    // Build available commands for the given actor context.
    // P53 stub: returns a minimal set (Wait, Inspect) for protocol testing.
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
};

} // namespace pathfinder::client_protocol
