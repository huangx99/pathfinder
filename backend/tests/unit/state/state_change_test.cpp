#include "pathfinder/state/state_change.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::state;
using namespace pathfinder::foundation;

void run_state_change_tests() {
    // Test empty StateChangeSet is valid
    {
        StateChangeSet set;
        assert(set.empty());
        assert(set.size() == 0);
        auto result = set.validateBasic();
        assert(result.is_ok());
    }

    // Test valid StateChange can be added
    {
        StateChangeSet set;
        StateChange change;
        change.change_id = StateChangeId("change_001");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("entity_001");
        change.field_path = "health";
        change.status = StateChangeStatus::Draft;
        change.reason = "test change";
        change.after_value = StateValue::makeInt(100);

        auto result = set.addChange(std::move(change));
        assert(result.is_ok());
        assert(!set.empty());
        assert(set.size() == 1);
    }

    // Test missing change_id fails
    {
        StateChange change;
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.field_path = "health";

        auto result = change.validateBasic();
        assert(result.is_error());
    }

    // Test NoOp without reason fails
    {
        StateChange change;
        change.change_id = StateChangeId("change_002");
        change.operation = StateChangeOperation::NoOp;
        change.domain = StateDomain::Entity;

        auto result = change.validateBasic();
        assert(result.is_error());
    }

    // Test NoOp with reason passes
    {
        StateChange change;
        change.change_id = StateChangeId("change_003");
        change.operation = StateChangeOperation::NoOp;
        change.domain = StateDomain::Entity;
        change.reason = "placeholder";

        auto result = change.validateBasic();
        assert(result.is_ok());
    }

    // Test Update without field_path fails
    {
        StateChange change;
        change.change_id = StateChangeId("change_004");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;

        auto result = change.validateBasic();
        assert(result.is_error());
    }

    // Test invalid change_id format fails
    {
        StateChange change;
        change.change_id = StateChangeId("invalid id!");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("entity_001");
        change.field_path = "health";

        auto result = change.validateBasic();
        assert(result.is_error());
    }

    // Test invalid target_id format fails
    {
        StateChange change;
        change.change_id = StateChangeId("change_bad_target");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("invalid id!");
        change.field_path = "health";

        auto result = change.validateBasic();
        assert(result.is_error());
    }

    // Test duplicate change_id fails
    {
        StateChangeSet set;
        StateChange change1;
        change1.change_id = StateChangeId("change_dup");
        change1.operation = StateChangeOperation::Update;
        change1.domain = StateDomain::Entity;
        change1.field_path = "health";
        set.addChange(std::move(change1));

        StateChange change2;
        change2.change_id = StateChangeId("change_dup");
        change2.operation = StateChangeOperation::Update;
        change2.domain = StateDomain::Entity;
        change2.field_path = "mana";

        auto result = set.addChange(std::move(change2));
        assert(result.is_error());
    }

    // Test Update without after_value fails
    {
        StateChange change;
        change.change_id = StateChangeId("change_no_after");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("entity_001");
        change.field_path = "health";

        auto result = change.validateBasic();
        assert(result.is_error());
    }

    // Test Update with Empty after_value fails
    {
        StateChange change;
        change.change_id = StateChangeId("change_empty_after");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("entity_001");
        change.field_path = "health";
        change.after_value = StateValue(); // Empty type

        auto result = change.validateBasic();
        assert(result.is_error());
    }

    // Test Update with valid after_value passes
    {
        StateChange change;
        change.change_id = StateChangeId("change_valid_after");
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("entity_001");
        change.field_path = "health";
        change.after_value = StateValue::makeInt(100);

        auto result = change.validateBasic();
        assert(result.is_ok());
    }

    // Test Create without after_value fails
    {
        StateChange change;
        change.change_id = StateChangeId("change_create_no_after");
        change.operation = StateChangeOperation::Create;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId("entity_001");

        auto result = change.validateBasic();
        assert(result.is_error());
    }

    std::cout << "state_change tests passed" << std::endl;
}
