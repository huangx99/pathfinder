#include "pathfinder/state/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::state;
using namespace pathfinder::foundation;

void run_state_types_tests() {
    // Test StateChangeOperation toString
    assert(toString(StateChangeOperation::Create) == "create");
    assert(toString(StateChangeOperation::Update) == "update");
    assert(toString(StateChangeOperation::Delete) == "delete");
    assert(toString(StateChangeOperation::Append) == "append");
    assert(toString(StateChangeOperation::Remove) == "remove");
    assert(toString(StateChangeOperation::NoOp) == "noop");

    // Test StateChangeOperation fromString
    {
        auto result = stateChangeOperationFromString("create");
        assert(result.is_ok());
        assert(result.value() == StateChangeOperation::Create);
    }
    {
        auto result = stateChangeOperationFromString("update");
        assert(result.is_ok());
        assert(result.value() == StateChangeOperation::Update);
    }
    {
        auto result = stateChangeOperationFromString("invalid");
        assert(result.is_error());
    }

    // Test StateChangeStatus toString
    assert(toString(StateChangeStatus::Draft) == "draft");
    assert(toString(StateChangeStatus::Validated) == "validated");
    assert(toString(StateChangeStatus::Applied) == "applied");
    assert(toString(StateChangeStatus::Rejected) == "rejected");

    // Test StateChangeStatus fromString
    {
        auto result = stateChangeStatusFromString("draft");
        assert(result.is_ok());
        assert(result.value() == StateChangeStatus::Draft);
    }
    {
        auto result = stateChangeStatusFromString("applied");
        assert(result.is_ok());
        assert(result.value() == StateChangeStatus::Applied);
    }
    {
        auto result = stateChangeStatusFromString("invalid");
        assert(result.is_error());
    }

    // Test StateDomain toString
    assert(toString(StateDomain::Unknown) == "unknown");
    assert(toString(StateDomain::World) == "world");
    assert(toString(StateDomain::Object) == "object");
    assert(toString(StateDomain::Entity) == "entity");
    assert(toString(StateDomain::Region) == "region");
    assert(toString(StateDomain::Knowledge) == "knowledge");
    assert(toString(StateDomain::Tribe) == "tribe");
    assert(toString(StateDomain::Civilization) == "civilization");
    assert(toString(StateDomain::System) == "system");

    // Test StateDomain fromString
    {
        auto result = stateDomainFromString("world");
        assert(result.is_ok());
        assert(result.value() == StateDomain::World);
    }
    {
        auto result = stateDomainFromString("knowledge");
        assert(result.is_ok());
        assert(result.value() == StateDomain::Knowledge);
    }
    {
        auto result = stateDomainFromString("invalid");
        assert(result.is_error());
    }
    // Unknown is not valid input
    {
        auto result = stateDomainFromString("unknown");
        assert(result.is_error());
    }

    std::cout << "state_types tests passed" << std::endl;
}
