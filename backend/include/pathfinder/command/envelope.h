#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/command/types.h"
#include "pathfinder/command/action_command.h"
#include <string>
#include <optional>

namespace pathfinder::command {

// Maximum length for idempotency_key and correlation_id
constexpr size_t kMaxKeyLength = 256;

// CommandEnvelope wraps a command with metadata
// P1 only: carries data and does basic validation
// Does NOT route to rule engine or modify state
struct CommandEnvelope {
    pathfinder::foundation::CommandId command_id;
    CommandSource source = CommandSource::Unknown;
    pathfinder::foundation::Tick issued_tick;
    std::optional<std::string> idempotency_key;
    std::optional<std::string> correlation_id;
    ActionCommand payload;

    // P1 basic validation
    // Checks: command_id valid, source not Unknown/Invalid, payload valid, key lengths
    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::command
