#include <cassert>
#include <iostream>
#include "pathfinder/command/types.h"

using namespace pathfinder::command;
using namespace pathfinder::foundation;

void run_command_types_tests() {
    // CommandSource toString
    {
        assert(toString(CommandSource::Unknown) == "unknown");
        assert(toString(CommandSource::Player) == "player");
        assert(toString(CommandSource::Ai) == "ai");
        assert(toString(CommandSource::System) == "system");
        assert(toString(CommandSource::Replay) == "replay");
        assert(toString(CommandSource::Test) == "test");
    }

    // CommandSource fromString
    {
        auto r = commandSourceFromString("player");
        assert(r.is_ok() && r.value() == CommandSource::Player);

        r = commandSourceFromString("ai");
        assert(r.is_ok() && r.value() == CommandSource::Ai);

        r = commandSourceFromString("system");
        assert(r.is_ok() && r.value() == CommandSource::System);

        r = commandSourceFromString("replay");
        assert(r.is_ok() && r.value() == CommandSource::Replay);

        r = commandSourceFromString("test");
        assert(r.is_ok() && r.value() == CommandSource::Test);

        r = commandSourceFromString("invalid");
        assert(r.is_error());
    }

    // CommandIntent toString
    {
        assert(toString(CommandIntent::Unknown) == "unknown");
        assert(toString(CommandIntent::Experiment) == "experiment");
        assert(toString(CommandIntent::Exploit) == "exploit");
        assert(toString(CommandIntent::Teach) == "teach");
        assert(toString(CommandIntent::AvoidRisk) == "avoid_risk");
        assert(toString(CommandIntent::Combine) == "combine");
        assert(toString(CommandIntent::Migrate) == "migrate");
        assert(toString(CommandIntent::Fight) == "fight");
    }

    // CommandIntent fromString
    {
        auto r = commandIntentFromString("unknown");
        assert(r.is_ok() && r.value() == CommandIntent::Unknown);

        r = commandIntentFromString("experiment");
        assert(r.is_ok() && r.value() == CommandIntent::Experiment);

        r = commandIntentFromString("avoid_risk");
        assert(r.is_ok() && r.value() == CommandIntent::AvoidRisk);

        r = commandIntentFromString("invalid");
        assert(r.is_error());
    }

    // ActionTargetType toString
    {
        assert(toString(ActionTargetType::None) == "none");
        assert(toString(ActionTargetType::Object) == "object");
        assert(toString(ActionTargetType::Entity) == "entity");
        assert(toString(ActionTargetType::Location) == "location");
        assert(toString(ActionTargetType::Region) == "region");
        assert(toString(ActionTargetType::Knowledge) == "knowledge");
        assert(toString(ActionTargetType::Group) == "group");
    }

    // ActionTargetType fromString
    {
        auto r = actionTargetTypeFromString("none");
        assert(r.is_ok() && r.value() == ActionTargetType::None);

        r = actionTargetTypeFromString("object");
        assert(r.is_ok() && r.value() == ActionTargetType::Object);

        r = actionTargetTypeFromString("knowledge");
        assert(r.is_ok() && r.value() == ActionTargetType::Knowledge);

        r = actionTargetTypeFromString("invalid");
        assert(r.is_error());
    }

    // ActionTargetRole toString
    {
        assert(toString(ActionTargetRole::Primary) == "primary");
        assert(toString(ActionTargetRole::Secondary) == "secondary");
        assert(toString(ActionTargetRole::Tool) == "tool");
        assert(toString(ActionTargetRole::Material) == "material");
        assert(toString(ActionTargetRole::Receiver) == "receiver");
        assert(toString(ActionTargetRole::Destination) == "destination");
    }

    // ActionTargetRole fromString
    {
        auto r = actionTargetRoleFromString("primary");
        assert(r.is_ok() && r.value() == ActionTargetRole::Primary);

        r = actionTargetRoleFromString("tool");
        assert(r.is_ok() && r.value() == ActionTargetRole::Tool);

        r = actionTargetRoleFromString("destination");
        assert(r.is_ok() && r.value() == ActionTargetRole::Destination);

        r = actionTargetRoleFromString("invalid");
        assert(r.is_error());
    }

    std::cout << "command_types_tests: all passed" << std::endl;
}
