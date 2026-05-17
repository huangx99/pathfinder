#pragma once

#include "pathfinder/agent/history_query.h"
#include "pathfinder/agent/replay_lock.h"
#include "pathfinder/agent/training_sample.h"
#include "pathfinder/protocol/envelope.h"
#include "pathfinder/protocol/types.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::protocol {

// --- Projection DTOs ---

struct AgentHistoryItemProjection {
    std::string record_id;
    std::string agent_id;
    std::string actor_id;
    int64_t tick = 0;
    uint64_t state_version_before = 0;
    uint64_t state_version_after = 0;

    std::string runtime_status;
    std::string decision_status;
    std::optional<std::string> selected_candidate_id;
    std::optional<std::string> selected_command_action_id;
    std::optional<std::string> command_id;
    std::optional<std::string> replay_record_id;

    std::optional<std::string> replay_lock_status;
    std::optional<std::string> training_sample_status;

    size_t state_change_count = 0;
    size_t event_count = 0;
    size_t error_count = 0;

    std::vector<std::string> phase_keys;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentHistoryProjection {
    std::string query_id;
    pathfinder::agent::AgentHistoryProjectionMode projection_mode = pathfinder::agent::AgentHistoryProjectionMode::Unknown;
    size_t total_matched = 0;
    bool truncated = false;
    std::vector<AgentHistoryItemProjection> items;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentReplayLockItemProjection {
    std::string agent_record_id;
    std::string agent_id;
    int64_t tick = 0;
    std::optional<std::string> command_id;
    std::optional<std::string> replay_record_id;
    std::string replay_lock_status;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentReplayLockProjection {
    std::string lock_set_id;
    bool all_replayable_without_policy = false;
    bool has_broken_entry = false;
    std::vector<AgentReplayLockItemProjection> entries;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentTrainingSampleProjection {
    std::string sample_id;
    std::string source_record_id;
    std::string status;
    std::string agent_id;
    std::string actor_id;
    int64_t tick = 0;
    std::optional<std::string> command_id;
    std::string runtime_status;
    std::string decision_status;
    std::string reward_status;
    std::string done_status;
    bool replay_locked = false;

    size_t observed_object_count = 0;
    size_t observed_threat_count = 0;
    size_t knowledge_claim_count = 0;
    size_t state_change_count = 0;
    size_t event_count = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- Projector Input ---

struct AgentHistoryProjectorInput {
    pathfinder::agent::AgentHistoryQueryResult query_result;
    const pathfinder::agent::AgentReplayLockSet* replay_lock_set = nullptr;
    const std::vector<pathfinder::agent::AgentTrainingSampleDraft>* training_samples = nullptr;
    pathfinder::agent::AgentHistoryProjectionMode projection_mode = pathfinder::agent::AgentHistoryProjectionMode::Public;
    bool include_debug_trace = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- Projector ---

class AgentHistoryProjector {
public:
    pathfinder::foundation::Result<AgentHistoryProjection> project(
        const AgentHistoryProjectorInput& input) const;
};

// --- Protocol Adapter Options ---

struct AgentProjectionProtocolOptions {
    std::string protocol_version = "1.0";
    std::optional<std::string> request_id;
    std::optional<std::string> correlation_id;
    std::optional<std::string> session_id;
};

// --- Protocol Adapter ---

class AgentProtocolProjectionAdapter {
public:
    ProtocolEnvelope buildHistoryEnvelope(
        const AgentHistoryProjection& projection,
        const AgentProjectionProtocolOptions& options = {}) const;

    ProtocolEnvelope buildReplayLockEnvelope(
        const AgentReplayLockProjection& projection,
        const AgentProjectionProtocolOptions& options = {}) const;

    ProtocolEnvelope buildTrainingSampleEnvelope(
        const AgentTrainingSampleProjection& projection,
        const AgentProjectionProtocolOptions& options = {}) const;
};

} // namespace pathfinder::protocol
