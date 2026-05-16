#pragma once

#include "pathfinder/agent/record_types.h"
#include "pathfinder/replay/command_replay.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>

namespace pathfinder::agent {

// AgentReplayBridge: 记录转复盘记录，不执行管线或回放
class AgentReplayBridge {
public:
    pathfinder::foundation::Result<pathfinder::replay::CommandReplayRecord>
    toCommandReplayRecord(const AgentTickRecord& record) const;

    pathfinder::foundation::Result<void> appendCommandReplayRecord(
        const AgentTickRecord& record,
        pathfinder::replay::CommandReplayLog& command_log) const;
};

// --- ReplayLockChecker ---

struct AgentReplayLockCheckResult {
    bool can_replay_without_policy = false;
    bool has_agent_record = false;
    bool has_command_record = false;
    std::vector<std::string> reason_keys;
};

// AgentReplayLockChecker: 检查回放锁定状态，不调用调度或回放
class AgentReplayLockChecker {
public:
    AgentReplayLockCheckResult check(
        const AgentTickRecord& agent_record,
        const pathfinder::replay::CommandReplayLog& command_log) const;
};

} // namespace pathfinder::agent
