#pragma once

#include "pathfinder/agent/record_types.h"
#include "pathfinder/replay/command_replay.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/foundation/result.h"
#include <vector>
#include <string>

namespace pathfinder::agent {

// --- ID Tag Types ---

struct AgentReplayLockSetIdTag {};
using AgentReplayLockSetId = pathfinder::foundation::StrongId<AgentReplayLockSetIdTag>;

// --- Enums ---

enum class AgentReplayLockStatus {
    Unknown,
    Locked,
    ExplainedOnly,
    Broken,
    Invalid
};

std::string toString(AgentReplayLockStatus status);
pathfinder::foundation::Result<AgentReplayLockStatus> agentReplayLockStatusFromString(const std::string& str);

enum class AgentReplayLockReason {
    Unknown,
    CommandRecordMatched,
    NoCommandExpected,
    CommandRecordMissing,
    CommandIdMismatch,
    AgentRecordInvalid,
    CommandRecordInvalid
};

std::string toString(AgentReplayLockReason reason);
pathfinder::foundation::Result<AgentReplayLockReason> agentReplayLockReasonFromString(const std::string& str);

// --- Data Contracts ---

struct AgentReplayLockEntry {
    AgentTickRecordId agent_record_id;
    AgentId agent_id;
    pathfinder::foundation::Tick tick;
    pathfinder::foundation::StateVersion state_version_before;
    pathfinder::foundation::StateVersion state_version_after;
    std::optional<pathfinder::foundation::DecisionId> decision_id;
    std::optional<pathfinder::foundation::CommandId> command_id;
    std::optional<pathfinder::replay::ReplayRecordId> replay_record_id;

    AgentReplayLockStatus status = AgentReplayLockStatus::Unknown;
    std::vector<AgentReplayLockReason> reasons;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentReplayLockSet {
    AgentReplayLockSetId lock_set_id;
    std::vector<AgentReplayLockEntry> entries;
    pathfinder::foundation::HashValue source_hash;

    pathfinder::foundation::Result<void> validateBasic() const;

    bool allReplayableWithoutPolicy() const;
    bool hasBrokenEntry() const;
};

// --- ID Helper ---

AgentReplayLockSetId makeAgentReplayLockSetId(
    pathfinder::foundation::Tick first_tick,
    pathfinder::foundation::Tick last_tick,
    size_t entry_count);

// --- Builder ---

struct AgentReplayLockBuildRequest {
    const AgentTickLog* agent_log = nullptr;
    const pathfinder::replay::CommandReplayLog* command_log = nullptr;
    pathfinder::foundation::HashValue source_hash;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class AgentReplayLockSetBuilder {
public:
    pathfinder::foundation::Result<AgentReplayLockSet> build(
        const AgentReplayLockBuildRequest& request) const;
};

} // namespace pathfinder::agent
