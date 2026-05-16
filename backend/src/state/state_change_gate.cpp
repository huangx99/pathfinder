#include "pathfinder/state/state_change_gate.h"
#include <set>

namespace pathfinder::state {

pathfinder::foundation::Result<StateChangeValidationReport> StateChangeGate::validate(
    const GameState& state,
    const StateChangeSet& change_set
) {
    StateChangeValidationReport report;

    // Check GameState basic validity
    auto state_result = state.validateBasic();
    if (state_result.is_error()) {
        for (const auto& err : state_result.errors()) {
            report.issues.push_back(StateChangeValidationIssue{
                err.code,
                err.message_key,
                "",
                ""
            });
        }
    }

    // Check StateChangeSet basic validity
    auto set_result = change_set.validateBasic();
    if (set_result.is_error()) {
        for (const auto& err : set_result.errors()) {
            report.issues.push_back(StateChangeValidationIssue{
                err.code,
                err.message_key,
                "",
                ""
            });
        }
    }

    // Check for duplicate target_id + field_path writes
    // This detects obvious conflicts where two changes write to the same path
    struct WriteKey {
        std::string target_id;
        std::string field_path;
        bool operator<(const WriteKey& other) const {
            if (target_id != other.target_id) return target_id < other.target_id;
            return field_path < other.field_path;
        }
    };

    std::set<WriteKey> seen_writes;
    for (const auto& change : change_set.changes()) {
        // Only check writes (Create, Update, Append) - Delete and Remove are different
        if (change.operation == StateChangeOperation::Create ||
            change.operation == StateChangeOperation::Update ||
            change.operation == StateChangeOperation::Append) {
            WriteKey key{change.target_id.value(), change.field_path};
            if (seen_writes.count(key) > 0) {
                report.issues.push_back(StateChangeValidationIssue{
                    pathfinder::foundation::ErrorCode::state_change_invalid,
                    "Duplicate write to same target_id + field_path",
                    change.field_path,
                    change.target_id.value()
                });
            }
            seen_writes.insert(key);
        }
    }

    return pathfinder::foundation::Result<StateChangeValidationReport>::ok(std::move(report));
}

} // namespace pathfinder::state
