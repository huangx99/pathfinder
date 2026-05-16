#pragma once

#include "pathfinder/foundation/result.h"
#include <string>
#include <optional>

namespace pathfinder::command {

// Command source types
enum class CommandSource {
    Unknown,
    Player,
    Ai,
    System,
    Replay,
    Test
};

std::string toString(CommandSource source);
pathfinder::foundation::Result<CommandSource> commandSourceFromString(const std::string& str);

// Command intent types
enum class CommandIntent {
    Unknown,
    Experiment,
    Exploit,
    Teach,
    AvoidRisk,
    Combine,
    Migrate,
    Fight
};

std::string toString(CommandIntent intent);
pathfinder::foundation::Result<CommandIntent> commandIntentFromString(const std::string& str);

// Action target types
enum class ActionTargetType {
    None,
    Object,
    Entity,
    Location,
    Region,
    Knowledge,
    Group
};

std::string toString(ActionTargetType type);
pathfinder::foundation::Result<ActionTargetType> actionTargetTypeFromString(const std::string& str);

// Action target roles
enum class ActionTargetRole {
    Primary,
    Secondary,
    Tool,
    Material,
    Receiver,
    Destination
};

std::string toString(ActionTargetRole role);
pathfinder::foundation::Result<ActionTargetRole> actionTargetRoleFromString(const std::string& str);

} // namespace pathfinder::command
