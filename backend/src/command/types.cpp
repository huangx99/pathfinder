#include "pathfinder/command/types.h"

namespace pathfinder::command {

// CommandSource
std::string toString(CommandSource source) {
    switch (source) {
        case CommandSource::Unknown: return "unknown";
        case CommandSource::Player:  return "player";
        case CommandSource::Ai:      return "ai";
        case CommandSource::System:  return "system";
        case CommandSource::Replay:  return "replay";
        case CommandSource::Test:    return "test";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<CommandSource> commandSourceFromString(const std::string& str) {
    if (str == "player")  return pathfinder::foundation::Result<CommandSource>::ok(CommandSource::Player);
    if (str == "ai")      return pathfinder::foundation::Result<CommandSource>::ok(CommandSource::Ai);
    if (str == "system")  return pathfinder::foundation::Result<CommandSource>::ok(CommandSource::System);
    if (str == "replay")  return pathfinder::foundation::Result<CommandSource>::ok(CommandSource::Replay);
    if (str == "test")    return pathfinder::foundation::Result<CommandSource>::ok(CommandSource::Test);
    return pathfinder::foundation::Result<CommandSource>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown CommandSource: " + str
        )
    );
}

// CommandIntent
std::string toString(CommandIntent intent) {
    switch (intent) {
        case CommandIntent::Unknown:    return "unknown";
        case CommandIntent::Experiment: return "experiment";
        case CommandIntent::Exploit:    return "exploit";
        case CommandIntent::Teach:      return "teach";
        case CommandIntent::AvoidRisk:  return "avoid_risk";
        case CommandIntent::Combine:    return "combine";
        case CommandIntent::Migrate:    return "migrate";
        case CommandIntent::Fight:      return "fight";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<CommandIntent> commandIntentFromString(const std::string& str) {
    if (str == "unknown")     return pathfinder::foundation::Result<CommandIntent>::ok(CommandIntent::Unknown);
    if (str == "experiment")  return pathfinder::foundation::Result<CommandIntent>::ok(CommandIntent::Experiment);
    if (str == "exploit")     return pathfinder::foundation::Result<CommandIntent>::ok(CommandIntent::Exploit);
    if (str == "teach")       return pathfinder::foundation::Result<CommandIntent>::ok(CommandIntent::Teach);
    if (str == "avoid_risk")  return pathfinder::foundation::Result<CommandIntent>::ok(CommandIntent::AvoidRisk);
    if (str == "combine")     return pathfinder::foundation::Result<CommandIntent>::ok(CommandIntent::Combine);
    if (str == "migrate")     return pathfinder::foundation::Result<CommandIntent>::ok(CommandIntent::Migrate);
    if (str == "fight")       return pathfinder::foundation::Result<CommandIntent>::ok(CommandIntent::Fight);
    return pathfinder::foundation::Result<CommandIntent>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown CommandIntent: " + str
        )
    );
}

// ActionTargetType
std::string toString(ActionTargetType type) {
    switch (type) {
        case ActionTargetType::None:      return "none";
        case ActionTargetType::Object:    return "object";
        case ActionTargetType::Entity:    return "entity";
        case ActionTargetType::Location:  return "location";
        case ActionTargetType::Region:    return "region";
        case ActionTargetType::Knowledge: return "knowledge";
        case ActionTargetType::Group:     return "group";
        default: return "none";
    }
}

pathfinder::foundation::Result<ActionTargetType> actionTargetTypeFromString(const std::string& str) {
    if (str == "none")      return pathfinder::foundation::Result<ActionTargetType>::ok(ActionTargetType::None);
    if (str == "object")    return pathfinder::foundation::Result<ActionTargetType>::ok(ActionTargetType::Object);
    if (str == "entity")    return pathfinder::foundation::Result<ActionTargetType>::ok(ActionTargetType::Entity);
    if (str == "location")  return pathfinder::foundation::Result<ActionTargetType>::ok(ActionTargetType::Location);
    if (str == "region")    return pathfinder::foundation::Result<ActionTargetType>::ok(ActionTargetType::Region);
    if (str == "knowledge") return pathfinder::foundation::Result<ActionTargetType>::ok(ActionTargetType::Knowledge);
    if (str == "group")     return pathfinder::foundation::Result<ActionTargetType>::ok(ActionTargetType::Group);
    return pathfinder::foundation::Result<ActionTargetType>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown ActionTargetType: " + str
        )
    );
}

// ActionTargetRole
std::string toString(ActionTargetRole role) {
    switch (role) {
        case ActionTargetRole::Primary:     return "primary";
        case ActionTargetRole::Secondary:   return "secondary";
        case ActionTargetRole::Tool:        return "tool";
        case ActionTargetRole::Material:    return "material";
        case ActionTargetRole::Receiver:    return "receiver";
        case ActionTargetRole::Destination: return "destination";
        default: return "primary";
    }
}

pathfinder::foundation::Result<ActionTargetRole> actionTargetRoleFromString(const std::string& str) {
    if (str == "primary")     return pathfinder::foundation::Result<ActionTargetRole>::ok(ActionTargetRole::Primary);
    if (str == "secondary")   return pathfinder::foundation::Result<ActionTargetRole>::ok(ActionTargetRole::Secondary);
    if (str == "tool")        return pathfinder::foundation::Result<ActionTargetRole>::ok(ActionTargetRole::Tool);
    if (str == "material")    return pathfinder::foundation::Result<ActionTargetRole>::ok(ActionTargetRole::Material);
    if (str == "receiver")    return pathfinder::foundation::Result<ActionTargetRole>::ok(ActionTargetRole::Receiver);
    if (str == "destination") return pathfinder::foundation::Result<ActionTargetRole>::ok(ActionTargetRole::Destination);
    return pathfinder::foundation::Result<ActionTargetRole>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown ActionTargetRole: " + str
        )
    );
}

} // namespace pathfinder::command
