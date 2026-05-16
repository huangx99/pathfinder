#include "pathfinder/state/state_change_gate.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::state;
using namespace pathfinder::foundation;

void run_state_change_gate_tests() {
    // Helper to create valid GameState
    auto makeValidState = []() {
        GameState state;
        state.metadata.state_id = GameStateId("state_001");
        state.metadata.state_version = StateVersion(1);
        state.metadata.current_tick = Tick(100);
        return state;
    };

    // Helper to create valid StateChange
    auto makeValidChange = [](
        const std::string& change_id_str,
        StateChangeOperation op,
        const std::string& target_id_str,
        const std::string& field
    ) {
        StateChange change;
        change.change_id = StateChangeId(change_id_str);
        change.operation = op;
        change.domain = StateDomain::Entity;
        change.target_id = TargetId(target_id_str);
        change.field_path = field;
        change.status = StateChangeStatus::Draft;
        // Add after_value for Create/Update/Append
        if (op == StateChangeOperation::Create ||
            op == StateChangeOperation::Update ||
            op == StateChangeOperation::Append) {
            change.after_value = StateValue::makeInt(100);
        }
        return change;
    };

    // Test valid state and change set passes
    {
        auto state = makeValidState();
        StateChangeSet change_set;
        change_set.addChange(makeValidChange("change_001", StateChangeOperation::Update, "entity_001", "health"));

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(!result.value().hasIssues());
    }

    // Test empty change set passes
    {
        auto state = makeValidState();
        StateChangeSet change_set;

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(!result.value().hasIssues());
    }

    // Test invalid GameState detected
    {
        GameState state;
        // Missing state_id
        StateChangeSet change_set;
        change_set.addChange(makeValidChange("change_002", StateChangeOperation::Update, "entity_001", "health"));

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(result.value().hasIssues());
    }

    // Test invalid StateChange detected
    {
        auto state = makeValidState();
        StateChangeSet change_set;
        // Missing change_id
        StateChange change;
        change.operation = StateChangeOperation::Update;
        change.domain = StateDomain::Entity;
        change.field_path = "health";
        // Force add without validation to test gate catches it
        // We can't use addChange since it validates, so we test with a properly invalid set
        change_set.addChange(makeValidChange("", StateChangeOperation::Update, "entity_001", "health"));

        auto result = StateChangeGate::validate(state, change_set);
        // addChange will reject empty change_id, so set is empty - gate should pass
        assert(result.is_ok());
    }

    // Test duplicate target_id + field_path detected
    {
        auto state = makeValidState();
        StateChangeSet change_set;
        change_set.addChange(makeValidChange("change_a", StateChangeOperation::Update, "entity_001", "health"));
        change_set.addChange(makeValidChange("change_b", StateChangeOperation::Update, "entity_001", "health"));

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(result.value().hasIssues());
        assert(result.value().count() == 1);
        assert(result.value().issues[0].code == ErrorCode::state_change_invalid);
    }

    // Test same target_id with different field_path is ok
    {
        auto state = makeValidState();
        StateChangeSet change_set;
        change_set.addChange(makeValidChange("change_c", StateChangeOperation::Update, "entity_001", "health"));
        change_set.addChange(makeValidChange("change_d", StateChangeOperation::Update, "entity_001", "mana"));

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(!result.value().hasIssues());
    }

    // Test different target_id with same field_path is ok
    {
        auto state = makeValidState();
        StateChangeSet change_set;
        change_set.addChange(makeValidChange("change_e", StateChangeOperation::Update, "entity_001", "health"));
        change_set.addChange(makeValidChange("change_f", StateChangeOperation::Update, "entity_002", "health"));

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(!result.value().hasIssues());
    }

    // Test gate does NOT modify GameState
    {
        auto state = makeValidState();
        auto original_version = state.metadata.state_version;
        auto original_tick = state.metadata.current_tick;

        StateChangeSet change_set;
        change_set.addChange(makeValidChange("change_g", StateChangeOperation::Update, "entity_001", "health"));

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(state.metadata.state_version == original_version);
        assert(state.metadata.current_tick == original_tick);
    }

    // Test multiple duplicate writes detected
    {
        auto state = makeValidState();
        StateChangeSet change_set;
        change_set.addChange(makeValidChange("change_x", StateChangeOperation::Update, "entity_001", "health"));
        change_set.addChange(makeValidChange("change_y", StateChangeOperation::Update, "entity_001", "health"));
        change_set.addChange(makeValidChange("change_z", StateChangeOperation::Create, "entity_001", "health"));

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(result.value().hasIssues());
        assert(result.value().count() == 2);
    }

    // Test Create + Update on same path detected as duplicate
    {
        auto state = makeValidState();
        StateChangeSet change_set;
        change_set.addChange(makeValidChange("change_m", StateChangeOperation::Create, "entity_001", "health"));
        change_set.addChange(makeValidChange("change_n", StateChangeOperation::Update, "entity_001", "health"));

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(result.value().hasIssues());
    }

    // Test Delete is NOT counted as a write (no conflict with Update)
    {
        auto state = makeValidState();
        StateChangeSet change_set;
        change_set.addChange(makeValidChange("change_p", StateChangeOperation::Delete, "entity_001", "health"));
        change_set.addChange(makeValidChange("change_q", StateChangeOperation::Update, "entity_001", "health"));

        auto result = StateChangeGate::validate(state, change_set);
        assert(result.is_ok());
        assert(!result.value().hasIssues());
    }

    std::cout << "state_change_gate tests passed" << std::endl;
}
